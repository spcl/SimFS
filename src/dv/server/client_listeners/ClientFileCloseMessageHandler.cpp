//
// Created by Pirmin Schmid on 16.04.17.
//

#include "ClientFileCloseMessageHandler.h"

#include <unistd.h>
#include <iostream>

#include "../DV.h"
#include "../../caches/filecaches/FileCache.h"
#include "../../caches/filecaches/FileDescriptor.h"

namespace dv {

	ClientFileCloseMessageHandler::ClientFileCloseMessageHandler(DV *dv, int socket,
																 const std::vector<std::string> &params)
			: MessageHandler(dv, socket, params) {

		if (params.size() < kNeededVectorSize) {
			LOG(ERROR, 0, "Insufficient number of arguments in params!");
			return;
		}

		filename_ = params[kFilenameIndex];
		initialized_ = true;
	}

	void ClientFileCloseMessageHandler::serve() {
		if (!initialized_) {
            LOG(ERROR, 0, "Incomplete initialization!");
			close(socket_);
			return;
		}

        LOG(CLIENT, 0, "Unlocking " + filename_);

		// only internal lookup is needed -> thus, no refresh later
		// note: there is no point in making a file a fresh MRU while handling its close message
		FileDescriptor *descriptor = dv_->getFileCachePtr()->internal_lookup_get(filename_);
		if (descriptor == nullptr) {
            LOG(ERROR, 0, "Trying to unlock a non-exisiting file: " + filename_);
			dv_->getFileCachePtr()->printStatus(&std::cerr);
			close(socket_);
			return;
		}


		descriptor->unlock();
		if (dv_->getConfigPtr()->dv_debug_output_on_) {
			std::cout << "   lock count after unlock: " << descriptor->getLockCount() << std::endl
					  << "   simulator lock? " << descriptor->isFileUsedBySimulator() << std::endl;
		}
		close(socket_);
	}

}
