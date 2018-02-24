//
// Created by Pirmin Schmid on 15.04.17.
//

#include "ClientFileOpenMessageHandler.h"

#include <unistd.h>
#include <iostream>
#include <memory>

#include "../DV.h"
#include "../ClientDescriptor.h"
#include "../../caches/filecaches/FileCache.h"
#include "../../caches/filecaches/FileDescriptor.h"
#include "../../simulator/Simulator.h"
#include "../../simulator/SimJob.h"
#include "../../toolbox/KeyValueStore.h"
#include "../../toolbox/StringHelper.h"


namespace dv {

	ClientFileOpenMessageHandler::ClientFileOpenMessageHandler(DV *dv, int socket,
															   const std::vector<std::string> &params)
			: MessageHandler(dv, socket, params) {

		if (params.size() < kNeededVectorSize) {
			std::cerr << "ClientFileOpenMessageHandler: insufficient number of arguments in params. Need "
					  << kNeededVectorSize << " got " << params.size() << std::endl;
			return;
		}

		filename_ = params[kFilenameIndex];

		try {
			appid_ = dv::stoid(params[kAppIdIndex]);
		} catch (const std::invalid_argument &ia) {
			std::cerr << "ERROR in ClientFileOpenMessageHandler: could not extract integer appid from params: "
					  << params[kAppIdIndex] << std::endl;
		}

		for (unsigned int i = kJobParamsStartIndex; i < params.size(); ++i) {
			jobparams_.push_back(params[i]);
		}

		initialized_ = true;
	}

	void ClientFileOpenMessageHandler::serve() {
		if (!initialized_) {
			std::cerr << "   -> cannot serve message due to incomplete initialization." << std::endl;
			close(socket_);
			return;
		}
        LOG(CLI_HANDLER, 0, "Client " + std::to_string(appid_) + " is opening " + filename_);
		dv_->getStatsPtr()->incTotal();

		// client open: we need the full get from the cache to trigger all actions in case of cache miss
		// note: use put() with a new descriptor in case a nullptr was returned and use refresh, if a descriptor was found


		// check whether client is known
		ClientDescriptor *clientDescriptor = dv_->findClientDescriptor(appid_);  

		if (clientDescriptor == nullptr) {
			// problem: unknown client
			std::cout << "ClientFileOpenMessageHandler: Warning: client with id " << appid_ << " not recognized!!!"
					  << std::endl;
			// TODO: do more? since this indicates a protocol violation (see: all clients should use Hello messages first)
			//       e.g. return here without further processing?
            return;
		} else {
			// ok. it is ok if fileDescriptor == nullptr
			bool can_client_read = clientDescriptor->handleOpen(filename_, jobparams_);
            
            if (can_client_read) sendAll(kLibReplyFileOpen);
            else sendAll(kLibReplyFileSim + dv_->getSimulatorPtr()->getMetadataFilename(filename_));
            
		}

		close(socket_);
	}

}
