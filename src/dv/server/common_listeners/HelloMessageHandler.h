//
// Created by Pirmin Schmid on 13.04.17.
//

#ifndef DV_SERVER_COMMON_LISTENERS_HELLOMESSAGEHANDLER_H_
#define DV_SERVER_COMMON_LISTENERS_HELLOMESSAGEHANDLER_H_

#include <string>
#include <vector>

#include "../../DVBasicTypes.h"
#include "../MessageHandler.h"


namespace dv {

	class HelloMessageHandler : public MessageHandler {
	public:
		HelloMessageHandler(DV *dv, int socket, const std::vector<std::string> &params);

		virtual void serve() override;


	private:
		static constexpr int kGniRankIndex = 1;
		static constexpr int kJobIdIndex = 2;
		static constexpr int kNeededVectorSize = 3;

		dv::id_type gnirank_;
		dv::id_type jobid_;
	};

}

#endif //DV_SERVER_COMMON_LISTENERS_HELLOMESSAGEHANDLER_H_
