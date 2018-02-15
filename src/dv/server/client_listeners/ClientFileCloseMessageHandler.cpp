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
			std::cerr << "ClientFileCloseMessageHandler: insufficient number of arguments in params. Need "
					  << kNeededVectorSize << " got " << params.size() << std::endl;
			return;
		}

		filename_ = params[kFilenameIndex];
		initialized_ = true;
	}

	void ClientFileCloseMessageHandler::serve() {
		if (!initialized_) {
			std::cerr << "   -> cannot serve message due to incomplete initialization." << std::endl;
			close(socket_);
			return;
		}

		std::cout << "Client is closing " << filename_ << std::endl;

		// only internal lookup is needed -> thus, no refresh later
		// note: there is no point in making a file a fresh MRU while handling its close message
		FileDescriptor *descriptor = dv_->getFileCachePtr()->internal_lookup_get(filename_);
		if (descriptor == nullptr) {
			std::cerr << "Error: ClientFileCloseMessageHandler: trying to unlock non-existing file " << filename_
					  << std::endl;
			dv_->getFileCachePtr()->printStatus(&std::cerr);
			close(socket_);
			return;
		}

		std::cout << "log_client_close " << filename_ << std::endl;

		descriptor->unlock();
		if (dv_->getConfigPtr()->dv_debug_output_on_) {
			std::cout << "   lock count after unlock: " << descriptor->getLockCount() << std::endl
					  << "   simulator lock? " << descriptor->isFileUsedBySimulator() << std::endl;
		}
		close(socket_);
	}

}
