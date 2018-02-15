//
// Created by Pirmin Schmid on 23.05.17.
//

#include "StatusRequestMessageHandler.h"

#include <unistd.h>
#include <iostream>

#include "../DV.h"

#include "../../toolbox/KeyValueStore.h"

namespace dv {

	StatusRequestMessageHandler::StatusRequestMessageHandler(DV *dv, int socket,
															 const std::vector<std::string> &params)
			: MessageHandler(dv, socket, params) {

		if (params.size() < kNeededVectorSize) {
			std::cerr << "StatusRequestMessageHandler: insufficient number of arguments in params. Need "
					  << kNeededVectorSize << " got " << params.size() << std::endl;
			return;
		}

		try {
			appid_ = dv::stoid(params[kAppIdIndex]);
		} catch (const std::invalid_argument &ia) {
			std::cerr << "ERROR in StatusRequestMessageHandler: could not extract integer appid from params: "
					  << params[kAppIdIndex] << std::endl;
		}

		initialized_ = true;
	}

	void StatusRequestMessageHandler::serve() {
		if (!initialized_) {
			std::cerr << "   -> cannot serve message due to incomplete initialization." << std::endl;
			close(socket_);
			return;
		}

		std::cout << "DV: received StatusRequest msg." << std::endl;

		sendAll(dv_->getStatusSummary().toString());

		close(socket_);
	}

}
