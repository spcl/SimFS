//
// Created by Pirmin Schmid on 20.04.17.
//


#include "MessageHandlerFactory.h"

#include <iostream>

#include "common_listeners/ExtendedApiMessageHandler.h"
#include "common_listeners/HelloMessageHandler.h"
#include "common_listeners/StatusRequestMessageHandler.h"
#include "common_listeners/StopServerMessageHandler.h"
#include "client_listeners/ClientFileOpenMessageHandler.h"
#include "client_listeners/ClientFileCloseMessageHandler.h"
#include "client_listeners/ClientVariableGetMessageHandler.h"
#include "simulator_listeners/SimulatorCheckpointCreateMessageHandler.h"
#include "simulator_listeners/SimulatorFileCloseMessageHandler.h"
#include "simulator_listeners/SimulatorVariablePutMessageHandler.h"
#include "simulator_listeners/SimulatorFileCreateMessageHandler.h"
#include "simulator_listeners/SimulatorFinalizeMessageHandler.h"


namespace dv {

	// this is additionally needed to get the strings into the binary
	constexpr char MessageHandlerFactory::kMsgHello[];
	constexpr char MessageHandlerFactory::kMsgFileOpen[];
	constexpr char MessageHandlerFactory::kMsgFileCloseClient[];
	constexpr char MessageHandlerFactory::kMsgVarGet[];
	constexpr char MessageHandlerFactory::kMsgFileCloseSim[];
	constexpr char MessageHandlerFactory::kMsgVarPut[];
	constexpr char MessageHandlerFactory::kMsgFileCreate[];
	constexpr char MessageHandlerFactory::kMsgFinalize[];
	constexpr char MessageHandlerFactory::kMsgCheckpointCreate[];
	constexpr char MessageHandlerFactory::kMsgExtendedApi[];
	constexpr char MessageHandlerFactory::kMsgStatusRequest[];
	constexpr char MessageHandlerFactory::kMsgStopServer[];

	std::unique_ptr<MessageHandler> MessageHandlerFactory::createMessageHandler(DV *dv, int socket,
															   MessageHandlerFactory::Origin origin,
															   const std::vector<std::string> &params) {

		std::string msg = params[0];
		switch (origin) {
			case kClient:
				if (msg == kMsgHello) {
					return std::make_unique<HelloMessageHandler>(dv, socket, params);
				} else if (msg == kMsgFileOpen) {
					return std::make_unique<ClientFileOpenMessageHandler>(dv, socket, params);
				} else if (msg == kMsgFileCloseClient) {
					return std::make_unique<ClientFileCloseMessageHandler>(dv, socket, params);
				} else if (msg == kMsgVarGet) {
					return std::make_unique<ClientVariableGetMessageHandler>(dv, socket, params);
				} else if (msg == kMsgFinalize) {
					// TODO if desired; not used yet
				} else if (msg == kMsgExtendedApi) {
					return std::make_unique<ExtendedApiMessageHandler>(dv, socket, params);
				} else if (msg == kMsgStatusRequest) {
					return std::make_unique<StatusRequestMessageHandler>(dv, socket, params);
				} else if (msg == kMsgStopServer) {
					return std::make_unique<StopServerMessageHandler>(dv, socket, params);
				}
				break;

			case kSimulator:
				if (msg == kMsgHello) {
					return std::make_unique<HelloMessageHandler>(dv, socket, params);
				} else if (msg == kMsgFileCloseSim) {
					return std::make_unique<SimulatorFileCloseMessageHandler>(dv, socket, params);
				} else if (msg == kMsgVarPut) {
					return std::make_unique<SimulatorVariablePutMessageHandler>(dv, socket, params);
				} else if (msg == kMsgFileCreate) {
					return std::make_unique<SimulatorFileCreateMessageHandler>(dv, socket, params);
				} else if (msg == kMsgCheckpointCreate) {
					return std::make_unique<SimulatorCheckpointCreateMessageHandler>(dv, socket, params);
				} else if (msg == kMsgFinalize) {
					return std::make_unique<SimulatorFinalizeMessageHandler>(dv, socket, params);
				} else if (msg == kMsgExtendedApi) {
					return std::make_unique<ExtendedApiMessageHandler>(dv, socket, params);
				}
				break;

			default:
				std::cerr << "unknown message handler origin" << std::endl;
		}

		// default: this is basically an error message to be logged (see serve() method in MessageHandler)
		return std::make_unique<MessageHandler>(dv, socket, params);
	}


	void MessageHandlerFactory::runMessageHandler(DV *dv, int socket, MessageHandlerFactory::Origin origin,
												  const std::vector<std::string> &params) {

		std::string msg = params[0];
		switch (origin) {
			case kClient:
				if (msg == kMsgHello) {
					HelloMessageHandler(dv, socket, params).serve();
				} else if (msg == kMsgFileOpen) {
					ClientFileOpenMessageHandler(dv, socket, params).serve();
				} else if (msg == kMsgFileCloseClient) {
					ClientFileCloseMessageHandler(dv, socket, params).serve();
				} else if (msg == kMsgVarGet) {
					ClientVariableGetMessageHandler(dv, socket, params).serve();
				} else if (msg == kMsgFinalize) {
					// TODO
				} else if (msg == kMsgExtendedApi) {
					ExtendedApiMessageHandler(dv, socket, params).serve();
				} else if (msg == kMsgStatusRequest) {
					StatusRequestMessageHandler(dv, socket, params).serve();
				} else if (msg == kMsgStopServer) {
					StopServerMessageHandler(dv, socket, params).serve();
				}
				break;

			case kSimulator:
				if (msg == kMsgHello) {
					HelloMessageHandler(dv, socket, params).serve();
				} else if (msg == kMsgFileCloseSim) {
					SimulatorFileCloseMessageHandler(dv, socket, params).serve();
				} else if (msg == kMsgVarPut) {
					SimulatorVariablePutMessageHandler(dv, socket, params).serve();
				} else if (msg == kMsgFileCreate) {
					SimulatorFileCreateMessageHandler(dv, socket, params).serve();
				} else if (msg == kMsgCheckpointCreate) {
					SimulatorCheckpointCreateMessageHandler(dv, socket, params).serve();
				} else if (msg == kMsgFinalize) {
					SimulatorFinalizeMessageHandler(dv, socket, params).serve();
				} else if (msg == kMsgExtendedApi) {
					ExtendedApiMessageHandler(dv, socket, params).serve();
				}
				break;

			default:
				std::cerr << "unknown message handler origin" << std::endl;
		}

		// separator
		//std::cout << "----------" << std::endl << std::endl;
	}


	void MessageHandlerFactory::runMessageHandler2(DV *dv, int socket, MessageHandlerFactory::Origin origin,
												  const std::vector<std::string> &params) {

		std::string msg = params[0];
		// no distinction of the origin of the message
		if (msg == kMsgHello) {
			HelloMessageHandler(dv, socket, params).serve();
		} else if (msg == kMsgFileOpen) {
			ClientFileOpenMessageHandler(dv, socket, params).serve();
		} else if (msg == kMsgFileCloseClient) {
			ClientFileCloseMessageHandler(dv, socket, params).serve();
		} else if (msg == kMsgVarGet) {
			ClientVariableGetMessageHandler(dv, socket, params).serve();
		} else if (msg == kMsgFileCloseSim) {
			SimulatorFileCloseMessageHandler(dv, socket, params).serve();
		} else if (msg == kMsgVarPut) {
			SimulatorVariablePutMessageHandler(dv, socket, params).serve();
		} else if (msg == kMsgFileCreate) {
			SimulatorFileCreateMessageHandler(dv, socket, params).serve();
		} else if (msg == kMsgCheckpointCreate) {
			SimulatorCheckpointCreateMessageHandler(dv, socket, params).serve();
		} else if (msg == kMsgFinalize) {
			SimulatorFinalizeMessageHandler(dv, socket, params).serve();
		} else if (msg == kMsgExtendedApi) {
			ExtendedApiMessageHandler(dv, socket, params).serve();
		} else if (msg == kMsgStatusRequest) {
			StatusRequestMessageHandler(dv, socket, params).serve();
		} else if (msg == kMsgStopServer) {
			StopServerMessageHandler(dv, socket, params).serve();
		}

		// separator
		//std::cout << "----------" << std::endl << std::endl;
	}

}
