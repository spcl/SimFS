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
        LOG(ERROR, 0, "Insufficient number of args!");
        return;
    }

    filename_ = params[kFilenameIndex];

    try {
        appid_ = dv::stoid(params[kAppIdIndex]);
    } catch (const std::invalid_argument &ia) {
        LOG(ERROR, 0, "Cannot extract appid!");
    }

    for (unsigned int i = kJobParamsStartIndex; i < params.size(); ++i) {
        jobparams_.push_back(params[i]);
    }

    initialized_ = true;
}

void ClientFileOpenMessageHandler::serve() {
    if (!initialized_) {
        LOG(ERROR, 0, "Incomplete initialization!");
        close(socket_);
        return;
    }
    LOG(CLIENT, 0, "Client " + std::to_string(appid_) + " is opening " + filename_);
    dv_->getStatsPtr()->incTotal();

    // client open: we need the full get from the cache to trigger all actions in case of cache miss
    // note: use put() with a new descriptor in case a nullptr was returned and use refresh, if a descriptor was found


    // check whether client is known
    ClientDescriptor *clientDescriptor = dv_->findClientDescriptor(appid_);

    if (clientDescriptor == nullptr) {
        // problem: unknown client
        LOG(WARNING, 0, "Client not recognized: " + std::to_string(appid_));
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
