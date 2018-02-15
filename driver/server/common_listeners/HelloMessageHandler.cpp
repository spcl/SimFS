//
// Created by Pirmin Schmid on 13.04.17.
//

#include "HelloMessageHandler.h"

#include <unistd.h>
#include <iostream>
#include <memory>

#include "../DV.h"
#include "../ClientDescriptor.h"
#include "../../simulator/SimJob.h"
#include "../../simulator/Simulator.h"


namespace dv {

	HelloMessageHandler::HelloMessageHandler(DV *dv, int socket, const std::vector<std::string> &params) :
			MessageHandler(dv, socket, params) {

		if (params.size() < kNeededVectorSize) {
			std::cerr << "HelloMessageHandler: insufficient number of arguments in params. Need "
					  << kNeededVectorSize << " got " << params.size() << std::endl;
			return;
		}

		try {
			gnirank_ = dv::stoid(params[kGniRankIndex]);
		} catch (const std::invalid_argument &ia) {
			std::cerr << "HelloMessageHandler: gnirank must be an integer value " << params[kGniRankIndex] << std::endl;
			return;
		}

		try {
			jobid_ = dv::stoid(params[kJobIdIndex]);
		} catch (const std::invalid_argument &ia) {
			std::cerr << "HelloMessageHandler: jobid must be an integer value " << params[kJobIdIndex] << std::endl;
			return;
		}

		initialized_ = true;
	}

	void HelloMessageHandler::serve() {
		if (!initialized_) {
			std::cerr << "   -> cannot serve message due to incomplete initialization." << std::endl;
			close(socket_);
			return;
		}

		std::cout << "DV: received HELLO msg. gni_rank " << gnirank_ << " jobid " << jobid_ << std::endl;

		SimJob *simJob = dv_->findSimJob(jobid_);
		if (simJob != nullptr) {
			std::cout << "Hello from a simulator. gni_rank " << gnirank_ << std::endl;

			// adjust gnirank & and send message
			simJob->setGniRank(gnirank_);
			dv::id_type rank = -1; // rank was already assigned when it was started
			std::string reply = dv_->getConfigPtr()->sim_result_path_ + kMsgDelimiter
								+ dv_->getConfigPtr()->sim_checkpoint_path_ + kMsgDelimiter
								+ std::to_string(rank);
			sendAll(reply);

		} else {
			std::cout << "Hello from a client!" << std::endl;

			// register new client
			dv::id_type rank = dv_->getNewRank();
			std::unique_ptr<ClientDescriptor> clientDescriptor = std::make_unique<ClientDescriptor>(dv_, rank);
			dv_->registerClient(rank, std::move(clientDescriptor));
			// do not access clientDescriptor here after this point

			// send message
			std::string reply = dv_->getConfigPtr()->sim_result_path_ + kMsgDelimiter
								+ dv_->getConfigPtr()->sim_checkpoint_path_ + kMsgDelimiter
								+ std::to_string(rank);
			sendAll(reply);
		}

		close(socket_);
	}

}
