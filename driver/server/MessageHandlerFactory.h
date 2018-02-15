//
// Created by Pirmin Schmid on 20.04.17.
//

#ifndef DV_SERVER_MESSAGEHANDLERFACTORY_H_
#define DV_SERVER_MESSAGEHANDLERFACTORY_H_

#include <memory>
#include <string>
#include <vector>

#include "../DVForwardDeclarations.h"
#include "MessageHandler.h"

namespace dv {

	class MessageHandlerFactory {
	public:
		enum Origin {kClient, kSimulator};

		// these constants must match the constants used
		// in DVLib client library
		// no string objects as static const/constexpr!
		static constexpr char kMsgHello[] = "0";
		static constexpr char kMsgFileOpen[] = "1";
		static constexpr char kMsgFileCloseClient[] = "2";
		static constexpr char kMsgVarGet[] = "3";
		static constexpr char kMsgFileCloseSim[] = "4";
		static constexpr char kMsgVarPut[] = "5";
		static constexpr char kMsgFileCreate[] = "6";
		static constexpr char kMsgFinalize[] = "7";
		static constexpr char kMsgCheckpointCreate[] = "8";

		static constexpr char kMsgExtendedApi[] = "E";

		// see dv_status and stop_dv apps for these messages
		static constexpr char kMsgStatusRequest[] = "S";
		static constexpr char kMsgStopServer[] = "X";

		/**
		 * The factory works well, but creates the short lived message handlers on the heap.
		 * Thus, the runMessageHandler may be a bit more efficient, which runs the handlers on the stack.
		 */
		static std::unique_ptr<MessageHandler> createMessageHandler(DV *dv, int socket, Origin origin, const std::vector<std::string> &params);

		static void runMessageHandler(DV *dv, int socket, Origin origin, const std::vector<std::string> &params);

		/**
		 * this dispatcher does no longer ckeck the origin of the message and allows access to all handlers
		 * independent of where the message came from.
		 */
		static void runMessageHandler2(DV *dv, int socket, Origin origin, const std::vector<std::string> &params);
	};

}

#endif //DV_SERVER_MESSAGEHANDLERFACTORY_H_
