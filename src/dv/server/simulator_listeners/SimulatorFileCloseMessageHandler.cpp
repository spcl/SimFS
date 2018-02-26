//
// Created by Pirmin Schmid on 16.04.17.
//

#include "SimulatorFileCloseMessageHandler.h"

#include <memory>
#include <unistd.h>
#include <iostream>
#include "../DV.h"
#include "../../caches/filecaches/FileCache.h"
#include "../../caches/filecaches/FileDescriptor.h"
#include "../../simulator/Simulator.h"
#include "../../toolbox/StringHelper.h"

namespace dv {

	SimulatorFileCloseMessageHandler::SimulatorFileCloseMessageHandler(DV *dv, int socket,
																	   const std::vector<std::string> &params)
			: MessageHandler(dv, socket, params) {

		if (params.size() < kNeededVectorSize) {
            LOG(ERROR, 0, "Insufficient number of arguments in params. (Expected: " + std::to_string(kNeededVectorSize) + "; Got: " + std::to_string(params.size()) + ")");
			return;
		}

		try {
			jobid_ = dv::stoid(params[kJobIdIndex]);
		} catch (const std::invalid_argument &ia) {
            LOG(ERROR, 0, "Cannot extract jobid from params: " + params[kJobIdIndex]);
		}

		filename_ = params[kFilenameIndex];

		try {
			filesize_ = dv::stoid(params[kFilesizeIndex]);
		} catch (const std::invalid_argument &ia) {
            LOG(ERROR, 0, "Cannot extract filesize from params: " + params[kFilesizeIndex]);
		}

		initialized_ = true;

	}

	void SimulatorFileCloseMessageHandler::serve() {
		if (!initialized_) {
            LOG(ERROR, 0, "cannot serve message due to incomplete initialization");
			close(socket_);
			return;
		}

        LOG(SIMULATOR, 0, "Simulator " + std::to_string(jobid_) + " created file " + filename_ + " (size: " + std::to_string(filesize_) + "B)");

		// lookup simulation
		SimJob *simjob = dv_->findSimJob(jobid_);
		if (simjob == nullptr) {
            LOG(ERROR, 0, "Job not recognized! (" + std::to_string(jobid_) + ")");
			close(socket_);
			return;
		}

		// handle only messages related to files in target range.
		// these checks shall not be removed
		dv::id_type t = dv_->getSimulatorPtr()->getResultFileType(filename_);
		if (t == 0) {
			if (dv_->getConfigPtr()->dv_debug_output_on_) {
                LOG(SIMULATOR, 0, "   unexpected file type: ignore.");
			}
			close(socket_);
			return;
		}

		// proper result file type -> do lookup
		FileDescriptor *fileDescriptor = dv_->getFileCachePtr()->internal_lookup_get(filename_);

		dv::id_type nr = dv_->getSimulatorPtr()->result2nr(filename_, t);
		if(!simjob->nrIsInSimulationRange(nr)) {
			if (dv_->getConfigPtr()->dv_debug_output_on_) {
                LOG(SIMULATOR, 0, "   file was not expected: ignore");
			}

			// but handle simulator lock count
			if (fileDescriptor != nullptr) {
				fileDescriptor->setFileUsedBySimulator(false);
			}

			close(socket_);
			return;
		}

		// continue with actual file handling
		if (fileDescriptor == nullptr) {
			// file was created that was not yet asked for, but is in target range
			// -> create new descriptor and add it to cache
			//
			// note: this code path should no longer happen with the new handling of file redirection
			// that adds all valid files in production already to the waiting list during simulator_file_create
			// event handling
			std::string fullpath = toolbox::StringHelper::joinPath(dv_->getConfigPtr()->sim_result_path_, filename_);
			std::unique_ptr<FileDescriptor> descriptor = std::make_unique<FileDescriptor>(filename_, fullpath);
			descriptor->setFileAvailable(true);
			descriptor->setFileUsedBySimulator(false);
			descriptor->setSize(filesize_);
			dv_->getFileCachePtr()->put(filename_, std::move(descriptor));
			// do not access descriptor anymore after this point

			// get actual pointer from stored descriptor in cache
			fileDescriptor = dv_->getFileCachePtr()->internal_lookup_get(filename_);
			if (fileDescriptor == nullptr) {
                LOG(ERROR, 0, "Error in adding " + filename_ + " to cache");
				dv_->getFileCachePtr()->printStatus(&std::cerr);
				close(socket_);
				return;
			}
		} else {
			// already known file was created
			// adjust & refresh
			fileDescriptor->setFileAvailable(true);
			fileDescriptor->setFileUsedBySimulator(false);
			fileDescriptor->setSize(filesize_);
			dv_->getFileCachePtr()->refresh(filename_);

			// again a lookup since pointer may have changed in some scenarios
			// (transfer of descriptor from internal waiting list to actual cache)
			fileDescriptor = dv_->getFileCachePtr()->internal_lookup_get(filename_);
			if (fileDescriptor == nullptr) {
                LOG(ERROR, 0, "Error in refreshing " + filename_ + " in cache");
				dv_->getFileCachePtr()->printStatus(&std::cerr);
				close(socket_);
				return;
			}
		}

		//std::cout << "log_sim_close " << filename_ << std::endl;

		// given asserts here:
		// - fileDescriptor != nullptr &&
		// - file descriptor up to date, incl. fileAvailable and is at MRU position &&
		// - fileDescriptor pointing to proper FileDescriptor object

		//std::cout << "   Job recognized. Handling fopen in simulations." << std::endl;
		simjob->handleSimulatorFileClose(filename_, fileDescriptor);

        //printf("filedescriptor: %p\n", fileDescriptor);

		// notifications
		// 1) update clientDescriptors (-> prepare for next requests)
		int client_notification_count = 0;
		for (auto client : fileDescriptor->getWaitingClientPtrs()) {
			client->handleNotification(simjob);
			++client_notification_count;
		}
		fileDescriptor->removeAllWaitingClientPtrs();

		// 2) sockets (-> actual communication to clients)
		int socket_notification_count = 0;
		for (auto socket : fileDescriptor->getNotificationSockets()) {
			sendAllToSocket(socket, kLibReplyFileOpen);
			++socket_notification_count;
		}

		// close the sockets in a separate loop
		for (auto socket : fileDescriptor->getNotificationSockets()) {
			close(socket);
		}

		fileDescriptor->removeAllNotificationSockets();

        LOG(SIMULATOR, 1, "  -> " + std::to_string(socket_notification_count) + " analyses have been notified");
		//std::cout << "   notified " << socket_notification_count << " DVLib sockets in "
		//		  << client_notification_count << " clients." << std::endl;

		close(socket_);
	}
}
