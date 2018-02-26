//
// Created by Pirmin Schmid on 16.04.17.
//

#include "SimulatorVariablePutMessageHandler.h"

#include <unistd.h>
#include <iostream>
#include <stdexcept>

namespace dv {

SimulatorVariablePutMessageHandler::SimulatorVariablePutMessageHandler(DV *dv, int socket,
        const std::vector<std::string> &params)
    : MessageHandler(dv, socket, params) {

    if (params.size() < kNeededVectorSize) {
        std::cerr << "SimulatorVariablePutMessageHandler: insufficient number of arguments in params. Need "
                  << kNeededVectorSize << " got " << params.size() << std::endl;
        return;
    }

    try {
        jobid_ = dv::stoid(params[kJobIdIndex]);
    } catch (const std::invalid_argument &ia) {
        std::cerr << "ERROR in SimulatorVariablePutMessageHandler: could not extract integer jobid from params: "
                  << params[kJobIdIndex] << std::endl;
    }

    filename_ = params[kFilenameIndex];

    // TODO: var id, dimension check and parsing of offsets and counts
    /* see
     self.dims = int(params[4]);
      if (self.dims>0):
        self.doff = [int(x) for x in params[5].split(",")];
        self.dcnt = [int(x) for x in params[6].split(",")];
      else:
        self.doff = []
        self.dcnt = []
     */

    initialized_ = true;

}

void SimulatorVariablePutMessageHandler::serve() {
    if (!initialized_) {
        std::cerr << "   -> cannot serve message due to incomplete initialization." << std::endl;
        close(socket_);
        return;
    }

    std::cout << "Simulator variable PUT request  " << filename_ << "; jobid " << jobid_ << std::endl;
    // print colored("   PUT request; vid:" + str(self.vid) + "; file: " + self.fname + "; dims: " + str(self.dims) + "; doff: " + str(self.doff) + "; dcnt: " + str(self.dcnt), "yellow")

    // TODO: do the actual variable handling

    // currently just close the socket
    close(socket_);
}

}
