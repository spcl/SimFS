//
// Created by Pirmin Schmid on 19.09.17.
//

#include "SimulatorCheckpointCreateMessageHandler.h"

#include "../DV.h"
#include <unistd.h>
#include <iostream>
#include <stdexcept>

#include "../../toolbox/FileSystemHelper.h"
#include "../../toolbox/StringHelper.h"

namespace dv {

SimulatorCheckpointCreateMessageHandler::SimulatorCheckpointCreateMessageHandler(DV *dv, int socket,
        const std::vector<std::string> &params)
    : MessageHandler(dv, socket, params) {

    if (params.size() < kNeededVectorSize) {
        std::cerr << "SimulatorCheckpointCreateMessageHandler: insufficient number of arguments in params. Need "
                  << kNeededVectorSize << " got " << params.size() << std::endl;
        return;
    }

    try {
        jobid_ = dv::stoid(params[kJobIdIndex]);
    } catch (const std::invalid_argument &ia) {
        std::cerr << "ERROR in SimulatorCheckpointCreateMessageHandler: could not extract integer jobid from params: "
                  << params[kJobIdIndex] << std::endl;
    }

    filename_ = params[kFilenameIndex];

    initialized_ = true;
}


/**
 * valid replies to DVLib are:
 * kLibReplyFileCreateAck
 * kLibReplyFileCreateKill
 * kLibReplyFileCreateRedirect:valid_absolute_redirect_path
 */

void SimulatorCheckpointCreateMessageHandler::serve() {
    if (!initialized_) {
        std::cerr << "   -> cannot serve message due to incomplete initialization." << std::endl;
        sendAll(kLibReplyFileCreateAck);
        close(socket_);
        return;
    }

    // lookup simjob
    SimJob *simjob = dv_->findSimJob(jobid_);
    if (simjob == nullptr) {
        std::cerr << "ERROR in SimulatorCheckpointCreateMessageHandler::serve(): Job " << jobid_
                  << " not recognized." << std::endl;


        sendAll(kLibReplyFileCreateAck);
        close(socket_);
        return;
    }


    // handle simjob that has received KILL signal
    if (simjob->hasToBeKilled()) {
        std::cout << "Killing SimJob! " << simjob->getCurrentNr() << " " << simjob->getSimStop() << std::endl;

        //Invalidate the killed job so future lookup calls will return false.
        simjob->invalidateJob();

        sendAll(kLibReplyFileCreateKill);
        close(socket_);
        return;
    }


    /**
     * note on the decision mechanism for needsRedirect (for checkpoint files):
     * At the moment no dynamic adding of new checkpoint files to the restart tree.
     * Thus, a simple "file exists" check is enough.
     * (see more sophisticated decision process for result files).
     */

    std::string fullpath = toolbox::StringHelper::joinPath(dv_->getConfigPtr()->sim_checkpoint_path_, filename_);
    bool needsRedirect = toolbox::FileSystemHelper::fileExists(fullpath);
    std::cout << "checkpoint create: " << filename_ << " (" << fullpath << ") needs redirect? "
              << (needsRedirect ? "YES" : "NO") << std::endl;

    if (needsRedirect) {
        std::string fullRedirectName = toolbox::StringHelper::joinPath(simjob->getRedirectPath_checkpoint(), filename_);
        std::string redirectPath = toolbox::FileSystemHelper::getDirname(fullRedirectName);

        if (!toolbox::FileSystemHelper::folderExists(redirectPath)) {
            std::cout << "creating redirect path " << redirectPath << std::endl;
            std::string command = "mkdir -p " + redirectPath;
            system(command.c_str());

            if (!toolbox::FileSystemHelper::folderExists(redirectPath)) {
                std::cout << "WARNING: redirect path " << redirectPath
                          << " could not be created. Using parent dir as alternate "
                          << simjob->getRedirectPath_result()
                          << " (increased risk of name collisions)" << std::endl;
                redirectPath = simjob->getRedirectPath_result();
                fullRedirectName = toolbox::StringHelper::joinPath(redirectPath, toolbox::FileSystemHelper::getBasename(filename_));
            }

        }

        std::cout << "Simulation " << jobid_ << " is creating CHECKPOINT file " << filename_
                  << " at redirected location " << fullRedirectName
                  << " to avoid overwriting an existing file" << std::endl;
        std::string reply(kLibReplyFileCreateRedirect);
        reply += std::string(kMsgDelimiter) + fullRedirectName;
        sendAll(reply);
    } else {
        sendAll(kLibReplyFileCreateAck);
    }
    close(socket_);
}

}
