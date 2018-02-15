//
// Created by Pirmin Schmid on 16.04.17.
//

#include "SimulatorFinalizeMessageHandler.h"

#include <unistd.h>
#include <iostream>

#include "../DV.h"

namespace dv {

	SimulatorFinalizeMessageHandler::SimulatorFinalizeMessageHandler(DV *dv, int socket, const std::vector<std::string> &params)
			: MessageHandler(dv, socket, params) {

		if (params.size() < kNeededVectorSize) {
			std::cerr << "SimulatorFileCloseMessageHandler: insufficient number of arguments in params. Need "
					  << kNeededVectorSize << " got " << params.size() << std::endl;
			return;
		}

		try {
			jobid_ = dv::stoid(params[kJobIdIndex]);
		} catch (const std::invalid_argument &ia) {
			std::cerr << "ERROR in SimulatorFileCloseMessageHandler: could not extract integer jobid from params: "
					  << params[kJobIdIndex] << std::endl;
		}

		initialized_ = true;
	}


	void SimulatorFinalizeMessageHandler::serve() {
		if (!initialized_) {
			std::cerr << "   -> cannot serve message due to incomplete initialization." << std::endl;
			close(socket_);
			return;
		}

		std::cout << "Simulation " << jobid_ << " is terminating" << std::endl;

		// lookup simulation
		SimJob *simjob = dv_->findSimJob(jobid_);
		if (simjob == nullptr) {
			std::cerr << "ERROR in SimulatorFinalizeMessageHandler::serve(): Job " << jobid_
					  << " not recognized." << std::endl;
			close(socket_);
			return;
		}

		simjob->terminate();
		dv_->removeJob(jobid_);

		close(socket_);
	}
}
