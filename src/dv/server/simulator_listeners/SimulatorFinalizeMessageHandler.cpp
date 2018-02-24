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
            LOG(ERROR, 0, "insufficient number of arguments in params!");
			return;
		}

		try {
			jobid_ = dv::stoid(params[kJobIdIndex]);
		} catch (const std::invalid_argument &ia) {
            LOG(ERROR, 0, "cannot extract integer jobid: " + params[kJobIdIndex]);
		}

		initialized_ = true;
	}


	void SimulatorFinalizeMessageHandler::serve() {
		if (!initialized_) {
            LOG(ERROR, 0, "Incomplete initialization!");
			close(socket_);
			return;
		}

        LOG(SIM_HANDLER, 0, "Simulation " + std::to_string(jobid_) + " is terminating");

		// lookup simulation
		SimJob *simjob = dv_->findSimJob(jobid_);
		if (simjob == nullptr) {
            LOG(ERROR, 0, "Job not recognized!");
			close(socket_);
			return;
		}

		simjob->terminate();
		dv_->removeJob(jobid_);

		close(socket_);
	}
}
