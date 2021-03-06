//
// Created by Pirmin Schmid on 16.04.17.
//

#include "ClientVariableGetMessageHandler.h"

#include <unistd.h>
#include <iostream>

#include "../DV.h"
#include "../ClientDescriptor.h"
#include "../../caches/filecaches/FileCache.h"
#include "../../caches/filecaches/FileDescriptor.h"


namespace dv {

ClientVariableGetMessageHandler::ClientVariableGetMessageHandler(DV *dv, int socket,
        const std::vector<std::string> &params)
    : MessageHandler(dv, socket, params) {

    if (params.size() < (kDimensionDetailsOn ? kNeededVectorSizeWithDetails : kNeededVectorSize)) {
        LOG(ERROR, 0, "Insufficient number of arguments!");
        return;
    }

    filename_ = params[kFilenameIndex];
    var_id_ = params[kVariableIdIndex];

    try {
        appid_ = dv::stoid(params[kAppIdIndex]);
    } catch (const std::invalid_argument &ia) {
        LOG(ERROR, 0, "appid must be an integer!");
        return;
    }

    if (kDimensionDetailsOn) {

        try {
            dimensions_ = dv::stodim(params[kDimensionsIndex]);
        } catch (const std::invalid_argument &ia) {
            LOG(ERROR, 0, "dimensions must be an integer!");
            return;
        }

        // TODO: rest of splitting (use delimiter constant defined in DV) and convert to offset values

        // TODO: remove this return as soon as this is done
        return;
    }

    initialized_ = true;
}

void ClientVariableGetMessageHandler::serve() {
    if (!initialized_) {
        LOG(ERROR, 0, "Incomplete initialization!");
        close(socket_);
        return;
    }

    ClientDescriptor *clientDescriptor = dv_->findClientDescriptor(appid_);
    if (clientDescriptor == nullptr) {
        LOG(ERROR, 0, "Cannot find client descriptor for this appid: " + std::to_string(appid_));
        // TODO: also here, do more? Since this is quite a severe protocol violation (i.e. no hello message before)
        close(socket_);
        return;
    }

    // variable access needs a full get() and refresh() later to adjust MRU order

    FileDescriptor *fileDescriptor = dv_->getFileCachePtr()->get(filename_);
    if (fileDescriptor == nullptr) {
        LOG(ERROR, 0, "Cannot find file descriptor for: " + filename_);
        // note: this is a security check. This should never happen for properly opened files (see locked).
        close(socket_);
        return;
    }

    // note: with the now implemented file overwrite protection (by redirection)
    // it is OK if fileDescriptor->isFileUsedBySimulator() == true
    // thus, no longer check for if (fileDescriptor->isFileAvailable() && !fileDescriptor->isFileUsedBySimulator())
    if (fileDescriptor->isFileAvailable()) {
        // send notification
        sendAll(kLibReplyFileOpen);
        close(socket_);
        if (dv_->getConfigPtr()->dv_debug_output_on_) {
            LOG(CLIENT, 1, "READ: data avail");
        }

        //std::cout << "log_get_var_avail " << filename_ << std::endl;
    } else {
        // let the client wait while the data is being simulated
        // note: message receive is blocking in DVLib

        // we do NOT close the socket now, but add it to the notification set of the file descriptor.
        // note: we need to store different sets for notification sockets and client descriptors
        // since clients may be concurrently waiting for lots of files
        // -> thus: client handler will then do the statistics part
        //          socket notification will then send the messages to the blocking DVLib routines
        fileDescriptor->appendNotificationSocket(socket_);
        fileDescriptor->appendWaitingClientPtr(clientDescriptor);



        if (dv_->getConfigPtr()->dv_debug_output_on_) {
            LOG(CLIENT, 1, "READ: data not avail. Will notify.");
        }

        //std::cout << "log_get_var_wait " << filename_ << std::endl;
    }

    // a variable get makes the file descriptor MRU
    //dv_->getFileCachePtr()->refresh(filename_);
}

}
