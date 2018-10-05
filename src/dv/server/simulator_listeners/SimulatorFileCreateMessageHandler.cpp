//
// Created by Pirmin Schmid on 16.04.17.
//

#include "SimulatorFileCreateMessageHandler.h"

#include "../DV.h"
#include <unistd.h>
#include <iostream>
#include <stdexcept>

#include "../../toolbox/FileSystemHelper.h"
#include "../../toolbox/StringHelper.h"
#include "../../toolbox/TimeHelper.h"

namespace dv {

SimulatorFileCreateMessageHandler::SimulatorFileCreateMessageHandler(DV *dv, int socket,
        const std::vector<std::string> &params)
    : MessageHandler(dv, socket, params) {

    if (params.size() < kNeededVectorSize) {
        LOG(ERROR, 0, "Insufficient number of arguments!");
        return;
    }

    try {
        jobid_ = dv::stoid(params[kJobIdIndex]);
    } catch (const std::invalid_argument &ia) {
        LOG(ERROR, 0, "Cannot extract jobid: " + params[kJobIdIndex]);
    }

    filename_ = params[kFilenameIndex];

    initialized_ = true;
}


/**
 * note: with the new fix for the race condition of file creation and file evictions from cache,
 * a reply message must be sent in all code paths. However, only after the relevant things
 * have been done (see set file used by simulator flag to protect from eviction).
 *
 * The default reply in case of an error in DV is ACK to avoid blocking the actual simulator.
 *
 *
 * note re handling of temporary redirects for already existing files to avoid overwriting them:
 * - we cannot just look at the scope/range of the given simjob
 *   thus, file system lookup is used whether file exists to answer this question
 * - additional handling later only for files within the range of the simjob
 * - to avoid filename conflicts, also relative paths of the requested file must be considered
 *   thus: some subfolders may be generated if needed for a valid path.
 * - no redirection is performend when in passive mode
 *
 * TODO: future option (soon):
 * - also handle create messages sent for restart/checkpoint files
 *
 *
 * valid replies to DVLib are:
 * kLibReplyFileCreateAck
 * kLibReplyFileCreateKill
 * kLibReplyFileCreateRedirect:valid_absolute_redirect_path
 */

void SimulatorFileCreateMessageHandler::serve() {
    if (!initialized_) {
        LOG(ERROR, 0, "Incomplete initialization!");
        sendAll(kLibReplyFileCreateAck);
        close(socket_);
        return;
    }

    // lookup simjob
    SimJob *simjob = dv_->findSimJob(jobid_);
    if (simjob == nullptr) {
        LOG(ERROR, 0, "Job not recognized! (" + std::to_string(jobid_) + ")");
        sendAll(kLibReplyFileCreateAck);
        close(socket_);
        return;
    }


    // handle simjob that has received KILL signal
    if (simjob->hasToBeKilled()) {
        LOG(INFO, 1, "Killing SimJob!");

        //Invalidate the killed job so future lookup calls will return false.
        simjob->invalidateJob();

        sendAll(kLibReplyFileCreateKill);
        close(socket_);
        return;
    }


    /**
     * note on the decision mechanism for needsRedirect:
     * One could simply test on whether the file physically exists.
     * However, some simulators (like FLASH) have an unlucky tendency to sometimes overshoot production of files.
     * Besides having non-overlapping partition boundaries, this was another reason to only include
     * files of [simstart, simstop), i.e. excluding simstop == simstart of next partition.
     * Thus, such files shall be overwritten by proper files. otherwise potentially wrong files will
     * be served to clients while proper files will always be redirected later.
     *
     * Therefore, decision for needsRedirect is made using the meta data in the fileDescriptor.
     * Both flags (file_available and used_by_simulator) are considered for this decision to
     * avoid overwriting files that are properly recognized to be within a valid simulator range.
     *
     * This also implies adding fileDescriptors of files of valid range that are not yet known to the cache
     * to the waiting list with set used_by_simulator flag. See implementation below.
     * note: since file_available is false, this will not add the object to the size limited cache but
     * to the unlimited waiting list (see cache implementations). SimulatorFileCloseMessagehandler will then
     * re-use this data structure while handling the close event and actually insert it into the cache
     * (including potential eviction operations) by using the cache.refresh operation. Thus, no memory
     * overhead but only a bit earlier allocated.
     */

    bool needsRedirect = false;

    FileDescriptor *fileDescriptor = dv_->getFileCachePtr()->internal_lookup_get(filename_);

    if (fileDescriptor != nullptr && !simjob->isPassive()) {
        // known file (which is valid part of a known simulator partition by definition)
        needsRedirect = fileDescriptor->isFileAvailable() || fileDescriptor->isFileUsedBySimulator();
    }

    // handle only messages related to files in target range.
    // these checks shall not be removed
    dv::id_type t = dv_->getSimulatorPtr()->getResultFileType(filename_);
    if (t != 0) {
        dv::id_type nr = dv_->getSimulatorPtr()->result2nr(filename_, t);
        if(simjob->nrIsInSimulationRange(nr)) {
            // fileDescriptor may be nullptr here and was semantically used before the following
            // new creation of a file descriptor that was added during redirect handling.
            simjob->handleSimulatorFileCreate(filename_, fileDescriptor);

            if (fileDescriptor == nullptr) {
                // file is being created that was not yet asked for, but is in target range
                // -> create new descriptor and add it to waiting list
                std::string fullpath = toolbox::StringHelper::joinPath(dv_->getConfigPtr()->sim_result_path_,
                                       filename_);
                std::unique_ptr<FileDescriptor> descriptor = std::make_unique<FileDescriptor>(filename_, fullpath);
                dv_->getFileCachePtr()->put(filename_, std::move(descriptor));
                // do not access descriptor anymore after this point

                // get actual pointer from stored descriptor in cache
                fileDescriptor = dv_->getFileCachePtr()->internal_lookup_get(filename_);
                if (fileDescriptor == nullptr) {
                    std::cerr << "ERROR in SimulatorFileCreateMessageHandler::serve() while adding " << filename_
                              << " to waiting list." << std::endl;
                    dv_->getFileCachePtr()->printStatus(&std::cerr);
                    sendAll(kLibReplyFileCreateAck);
                    close(socket_);
                    return;
                }
            }
        } else {
            if (dv_->getConfigPtr()->dv_debug_output_on_) {
                std::cout << "   file was not asked for: ignore" << std::endl;
            }
        }

        toolbox::TimeHelper::time_point_type now = toolbox::TimeHelper::now();
        double time = toolbox::TimeHelper::seconds(dv_->start_time_, now);
        LOG(SIMULATOR, 1, "Simulator " + std::to_string(jobid_) + " starts creating file " + filename_ + "; time: " + std::to_string(time));

    } else {
        if (dv_->getConfigPtr()->dv_debug_output_on_) {
            std::cout << "   ignore this file type" << std::endl;
        }
    }

    // this must come after the redirect decision check above (otherwise the check will always be true)
    if (fileDescriptor != nullptr) {
        fileDescriptor->setFileUsedBySimulator(true);
        // this will protect the file from eviction and overwriting
    }


    if (needsRedirect) {
        std::string fullRedirectName = toolbox::StringHelper::joinPath(simjob->getRedirectPath_result(), filename_);
        std::string redirectPath = toolbox::FileSystemHelper::getDirname(fullRedirectName);

        if (!toolbox::FileSystemHelper::folderExists(redirectPath)) {
            LOG(INFO, 1, "creating redirect path " + redirectPath);
            std::string command = "mkdir -p " + redirectPath;
            system(command.c_str());

            if (!toolbox::FileSystemHelper::folderExists(redirectPath)) {
                LOG(WARNING, 0, "failed to create redirect path (" + redirectPath + "). Falling back to " + simjob->getRedirectPath_result() + ". This may cause collisions.");
                redirectPath = simjob->getRedirectPath_result();
                fullRedirectName = toolbox::StringHelper::joinPath(redirectPath, toolbox::FileSystemHelper::getBasename(filename_));
            }

        }

        //LOG(SIMULATOR, 1, "Simulation " + std::to_string(jobid_) + " is creating " + filename_);
        /*
        std::cout << "Simulation " << jobid_ << " is creating file " << filenam
        		  << " at redirected location " << fullRedirectName
        		  << " to avoid overwriting an existing file" << std::endl;
        std::cout << "log_create " << filename_ << std::endl;
        std::cout << "log_redirect " << fullRedirectName << std::endl;
        */
        std::string reply(kLibReplyFileCreateRedirect);
        reply += std::string(kMsgDelimiter) + fullRedirectName;
        sendAll(reply);
    } else {
        //LOG(SIMULATOR, 1, "Simulation " + std::to_string(jobid_) + " is creating " + filename_);
        //std::cout << "Simulation " << jobid_ << " is creating file " << filename_ << std::endl;
        //std::cout << "log_create " << filename_ << std::endl;
        sendAll(kLibReplyFileCreateAck);
    }
    close(socket_);
}

}
