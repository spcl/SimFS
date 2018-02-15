//
// Created by Pirmin Schmid on 26.04.17.
//

#ifndef DV_SERVER_COMMON_LISTENERS_STOPSERVERMESSAGEHANDLER_H_
#define DV_SERVER_COMMON_LISTENERS_STOPSERVERMESSAGEHANDLER_H_


#include <string>
#include <vector>

#include "../../DVBasicTypes.h"
#include "../MessageHandler.h"


namespace dv {

	class StopServerMessageHandler : public MessageHandler {
	public:
		StopServerMessageHandler(DV *dv, int socket, const std::vector<std::string> &params);

		virtual void serve() override;


	private:
		static constexpr int kAppIdIndex = 1;
		static constexpr int kNeededVectorSize = 2;

		dv::id_type appid_;
	};

}

#endif //DV_SERVER_COMMON_LISTENERS_STOPSERVERMESSAGEHANDLER_H_
