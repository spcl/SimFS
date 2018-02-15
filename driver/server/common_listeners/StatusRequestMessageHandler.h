//
// Created by Pirmin Schmid on 23.05.17.
//

#ifndef DV_SERVER_COMMON_LISTENERS_STATUSREQUESTMESSAGEHANDLER_H_
#define DV_SERVER_COMMON_LISTENERS_STATUSREQUESTMESSAGEHANDLER_H_

#include <string>
#include <vector>

#include "../../DVBasicTypes.h"
#include "../MessageHandler.h"

namespace dv {

	class StatusRequestMessageHandler : public MessageHandler {
	public:
		StatusRequestMessageHandler(DV *dv, int socket, const std::vector<std::string> &params);

		virtual void serve() override;


	private:
		static constexpr int kAppIdIndex = 1;
		static constexpr int kNeededVectorSize = 2;

		dv::id_type appid_;
	};

}

#endif //DV_SERVER_COMMON_LISTENERS_STATUSREQUESTMESSAGEHANDLER_H_
