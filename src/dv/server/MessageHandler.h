//
// Created by Pirmin Schmid on 13.04.17.
//

#ifndef DV_SERVER_MESSAGEHANDLER_H_
#define DV_SERVER_MESSAGEHANDLER_H_

#include <string>
#include <vector>

#include "../DVForwardDeclarations.h"

namespace dv {

	class MessageHandler {
	public:
		// these constants must match the constants used
		// in DVLib client library
		// no string objects as static const/constexpr!
		static constexpr char kLibReplyFileOpen[] = "0";
		static constexpr char kLibReplyFileSim[] = "1";
		static constexpr char kLibReplyRDMA[] = "2";

		static constexpr char kLibReplyFileCreateAck[] = "0";
        static constexpr char kLibReplyFileCreateKill[] = "1";
		static constexpr char kLibReplyFileCreateRedirect[] = "2";

		static constexpr char kMsgDelimiter[] = ":";
		static constexpr char kParamDelimiter[] = ";";
		static constexpr char kParamAssignSymbol[] = "=";
		static constexpr char kVarDimensionDelimiter[] = ",";

		MessageHandler(DV *dv, int socket, const std::vector<std::string> &params);

		virtual ~MessageHandler() {}

		virtual void serve();


	protected:
		DV *dv_;
		int socket_;
		bool initialized_;

		bool sendAll(const std::string &reply);

		bool sendAllToSocket(int socket, const std::string &reply);
	};

}

#endif //DV_SERVER_MESSAGEHANDLER_H_
