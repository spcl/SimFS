//
// Created by Pirmin Schmid on 23.07.17.
//

#include "ExtendedApiMessageHandler.h"

#include <unistd.h>
#include <iostream>
#include <memory>

#include "../DV.h"

namespace dv {

ExtendedApiMessageHandler::ExtendedApiMessageHandler(DV *dv, int socket,
        const std::vector<std::string> &params)
    : MessageHandler(dv, socket, params) {

    if (params.size() < kNeededVectorSize) {
        std::cerr << "ExtendedApiMessageHandler: insufficient number of arguments in params. Need "
                  << kNeededVectorSize << " got " << params.size() << std::endl;
        return;
    }

    try {
        appid_ = dv::stoid(params[kAppIdIndex]);
    } catch (const std::invalid_argument &ia) {
        std::cerr << "ERROR in ExtendedApiMessageHandler: could not extract integer appid from params: "
                  << params[kAppIdIndex] << std::endl;
    }

    try {
        api_function_ = dv::stoid(params[kApiFunctionIndex]);
    } catch (const std::invalid_argument &ia) {
        std::cerr << "ERROR in ExtendedApiMessageHandler: could not extract integer api_function from params: "
                  << params[kApiFunctionIndex] << std::endl;
    }

    try {
        api_arguments_count_ = dv::stoid(params[kApiArgumentsCountIndex]);
    } catch (const std::invalid_argument &ia) {
        std::cerr << "ERROR in ExtendedApiMessageHandler: could not extract integer api_arguments_count from params: "
                  << params[kApiArgumentsCountIndex] << std::endl;
    }

    if (params.size() < (kApiFirstArgumentIndex + api_arguments_count_))  {
        std::cerr << "ExtendedApiMessageHandler: insufficient number of arguments in params. Need "
                  << (kApiFirstArgumentIndex + api_arguments_count_) << " got " << params.size() << std::endl;
        return;
    }

    for (int i = 0; i < api_arguments_count_; ++i) {
        api_arguments_.push_back(params[kApiFirstArgumentIndex + i]);
    }

    initialized_ = true;
}

void ExtendedApiMessageHandler::serve() {
    if (!initialized_) {
        std::cerr << "   -> cannot serve message due to incomplete initialization." << std::endl;
        close(socket_);
        return;
    }

    std::cout << "DV: received ExtendedApiMessageHandler msg:" << std::endl
              << "appid " << appid_ << ", API function nr " << api_function_
              << ", " << api_arguments_count_ << " arguments" << std::endl;

    for (int i = 0; i < api_arguments_count_; ++i) {
        std::cout << "  argument " << i << ": " << api_arguments_[i] << std::endl;
    }

    switch (api_function_) {
    case kHandleSetInfo:
        handle_set_info();
        break;
    case kHandleGetInfo:
        handle_get_info();
        break;
    case kHandleRequestRange:
        handle_request_range();
        break;
    case kHandleTestFile:
        handle_test_file();
        break;
    case kHandleStatus:
        handle_status();
        break;
    default:
        std::cerr << "Unknown extended API function number " << api_function_
                  << "from appid " << appid_ << " with arguments " << std::endl;
        for (int i = 0; i < api_arguments_count_; ++i) {
            std::cerr << "  argument " << i << ": " << api_arguments_[i] << std::endl;
        }
    }


    close(socket_);
}

void ExtendedApiMessageHandler::handle_set_info() {
    // TODO
    // note: only a secure subset of all status should be available to be set from outside

    if (api_arguments_count_ < 2) {
        std::cerr << "not enough arguments for handle_set_info(). Need 2." << std::endl;
        sendAll("-1");
        return;
    }

    // currently for testing purpose
    dv::id_type value = 0;
    try {
        value = dv::stoid(api_arguments_[1]);
    } catch (const std::invalid_argument &ia) {
        std::cerr << "ERROR in ExtendedApiMessageHandler::handle_set_info(): could not extract integer value from argument1: "
                  << api_arguments_[1] << std::endl;
        sendAll("-1");
        return;
    }

    dv_->extendedApiSetInfo(api_arguments_[0], value);
    sendAll("0");
}

void ExtendedApiMessageHandler::handle_get_info() {
    // TODO
    // note: only a secure subset of all status should be available to be read from outside

    if (api_arguments_count_ < 2) {
        std::cerr << "not enough arguments for handle_get_info(). Need 2." << std::endl;
        sendAll("-1");
        return;
    }

    // currently for testing purpose
    dv::id_type default_value = 0;
    try {
        default_value = dv::stoid(api_arguments_[1]);
    } catch (const std::invalid_argument &ia) {
        std::cerr << "ERROR in ExtendedApiMessageHandler::handle_get_info(): could not extract integer value from argument1: "
                  << api_arguments_[1] << std::endl;
        sendAll("-1");
        return;
    }

    // currently for testing purpose
    sendAll(std::to_string(dv_->extendedApiGetInfo(api_arguments_[0], default_value)));
}

void ExtendedApiMessageHandler::handle_request_range() {
    ClientDescriptor *clientDescriptor = dv_->findClientDescriptor(appid_);
    if (clientDescriptor == nullptr) {
        std::cerr << "ERROR in ExtendedApiMessageHandler::handle_request_range(): clientdescriptor not found: " << std::endl;
        sendAll("-1");
        return;
    }

    if (api_arguments_count_ < 4) {
        std::cerr << "not enough arguments for handle_request_range(). Need 4." << std::endl;
        sendAll("-1");
        return;
    }

    dv::id_type flag = 0;
    try {
        flag = dv::stoid(api_arguments_[0]);
    } catch (const std::invalid_argument &ia) {
        std::cerr << "ERROR in ExtendedApiMessageHandler::handle_request_range(): could not extract integer value from argument0: "
                  << api_arguments_[0] << std::endl;
        sendAll("-1");
        return;
    }

    dv::id_type stride = 0;
    try {
        stride = dv::stoid(api_arguments_[3]);
    } catch (const std::invalid_argument &ia) {
        std::cerr << "ERROR in ExtendedApiMessageHandler::handle_request_range(): could not extract integer value from argument3: "
                  << api_arguments_[3] << std::endl;
        sendAll("-1");
        return;
    }

    std::unique_ptr<ClientDescriptor::RangeRequest> rangeRequest =
        std::make_unique<ClientDescriptor::RangeRequest>(api_arguments_[1], api_arguments_[2], stride);

    clientDescriptor->handleRangeRequest(flag, std::move(rangeRequest));
    sendAll("0");
}

void ExtendedApiMessageHandler::handle_test_file() {
    // TODO

    std::cerr << "ExtendedApiMessageHandler::handle_test_file() not yet implemented." << std::endl;
    sendAll("-1");
}

void ExtendedApiMessageHandler::handle_status() {
    // TODO

    std::cerr << "ExtendedApiMessageHandler::handle_status() not yet implemented." << std::endl;
    sendAll("-1");
}
}
