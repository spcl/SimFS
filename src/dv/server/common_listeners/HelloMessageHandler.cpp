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
        LOG(ERROR, 0, "Insufficient number of arugments!");
        return;
    }

    try {
        gnirank_ = dv::stoid(params[kGniRankIndex]);
    } catch (const std::invalid_argument &ia) {
        LOG(ERROR, 0, "gnirank must be an integer!");
        return;
    }

    try {
        jobid_ = dv::stoid(params[kJobIdIndex]);
    } catch (const std::invalid_argument &ia) {
        LOG(ERROR, 0, "jobid must be an integer!");
        return;
    }

    initialized_ = true;
}

void HelloMessageHandler::serve() {
    if (!initialized_) {
        LOG(ERROR, 0, "Incomplete initialization!");
        close(socket_);
        return;
    }


    //std::cout << "DV: received HELLO msg. gni_rank " << gnirank_ << " jobid " << jobid_ << std::endl;

    SimJob *simJob = dv_->findSimJob(jobid_);
    if (simJob != nullptr || dv_->isPassive()) {

        /* we assume it's a simulator */
        if (dv_->isPassive()){
            std::unique_ptr<SimJob> simjob_uptr = dv_->getSimulatorPtr()->generatePassiveSimJob(jobid_);
            simJob = simjob_uptr.get();
            dv_->indexJob(jobid_, std::move(simjob_uptr));
        }

        toolbox::TimeHelper::time_point_type now = toolbox::TimeHelper::now();
        double time = toolbox::TimeHelper::milliseconds(dv_->start_time_, now);
        LOG(SIMULATOR, 1, "Hello from a simulator: " + std::to_string(time));
        LOG(SIMULATOR, 1, "[EVENT][" + std::to_string(jobid_) + "] SIMHELLO: " + std::to_string(time));

        // adjust gnirank & and send message
        simJob->setGniRank(gnirank_);
        dv::id_type rank = -1; // rank was already assigned when it was started
        std::string reply = dv_->getConfigPtr()->sim_result_path_ + kMsgDelimiter
                            + dv_->getConfigPtr()->sim_checkpoint_path_ + kMsgDelimiter
                            + std::to_string(rank);
        sendAll(reply);

    } else {
        /* standard case: we assume it's a client */
        LOG(CLIENT, 1, "Hello from a client!");

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
