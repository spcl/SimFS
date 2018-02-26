//
// Created by Pirmin Schmid on 26.04.17.
//

#include "StopServerMessageHandler.h"

#include <unistd.h>
#include <iostream>

#include "../DV.h"

namespace dv {

StopServerMessageHandler::StopServerMessageHandler(DV *dv, int socket, const std::vector<std::string> &params)
    : MessageHandler(dv, socket, params) {

    if (params.size() < kNeededVectorSize) {
        std::cerr << "StopServerMessageHandler: insufficient number of arguments in params. Need "
                  << kNeededVectorSize << " got " << params.size() << std::endl;
        return;
    }

    try {
        appid_ = dv::stoid(params[kAppIdIndex]);
    } catch (const std::invalid_argument &ia) {
        std::cerr << "ERROR in StopServerMessageHandler: could not extract integer appid from params: "
                  << params[kAppIdIndex] << std::endl;
    }

    initialized_ = true;
}

void StopServerMessageHandler::serve() {
    if (!initialized_) {
        std::cerr << "   -> cannot serve message due to incomplete initialization." << std::endl;
        close(socket_);
        return;
    }

    std::cout << "StopServerMessageHandler: Message received. Server will stop." << std::endl;

    dv_->quit();

    close(socket_);
}

}
