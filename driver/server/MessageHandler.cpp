//
// Created by Pirmin Schmid on 13.04.17.
//

#include "MessageHandler.h"

#include <sys/socket.h>
#include <iostream>
#include "DV.h"
#include "DVConfig.h"

namespace dv {

	// this is additionally needed to get the strings into the binary
	constexpr char MessageHandler::kLibReplyFileOpen[];
	constexpr char MessageHandler::kLibReplyFileSim[];
	constexpr char MessageHandler::kLibReplyRDMA[];

	constexpr char MessageHandler::kLibReplyFileCreateAck[];
	constexpr char MessageHandler::kLibReplyFileCreateKill[];
	constexpr char MessageHandler::kLibReplyFileCreateRedirect[];

	constexpr char MessageHandler::kMsgDelimiter[];
	constexpr char MessageHandler::kParamDelimiter[];
	constexpr char MessageHandler::kParamAssignSymbol[];
	constexpr char MessageHandler::kVarDimensionDelimiter[];

	MessageHandler::MessageHandler(DV *dv, int socket, const std::vector<std::string> &params) :
			dv_(dv), socket_(socket) {

		// note: nothing is done with params here at the moment,
		// in particular, it is not stored
		// see parsing of params in the individual message handlers
		// due to different message formats

		if (dv_->getConfigPtr()->dv_debug_output_on_) {
			std::cout << "Messagehandler: socket " << socket << ", " << params.size() << " params: ";
			for (const auto &p : params) {
				std::cout << p << "; ";
			}
			std::cout << std::endl;
		}
	}

	void MessageHandler::serve() {
		std::cerr << "ERROR: This is a MessageHandler object! This should never have been built. "
				"Check MessageHandlerFactory and/or messages sent to DV." << std::endl;
	}

	bool MessageHandler::sendAll(const std::string &reply) {
		return sendAllToSocket(socket_, reply);
	}

	bool MessageHandler::sendAllToSocket(int socket, const std::string &reply) {
		if (dv_->getConfigPtr()->dv_debug_output_on_) {
			std::cout << "Messagehandler: sending on socket " << socket << ": " << reply << std::endl;
		}

		const char *ptr = reply.c_str();
		size_t len = reply.size();
		while (len > 0) {
			ssize_t sent = send(socket, ptr, len, 0);
			if (sent < 1) {
				return false;
			}
			ptr += sent;
			len -= sent;
		}
		return true;
	}

}
