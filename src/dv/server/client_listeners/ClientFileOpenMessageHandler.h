//
// Created by Pirmin Schmid on 15.04.17.
//

#ifndef DV_SERVER_CLIENT_LISTENERS_CLIENTFILEOPENMESSAGEHANDLER_H_
#define DV_SERVER_CLIENT_LISTENERS_CLIENTFILEOPENMESSAGEHANDLER_H_

#include <string>
#include <vector>

#include "../../DVBasicTypes.h"
#include "../MessageHandler.h"


namespace dv {

	class ClientFileOpenMessageHandler : public MessageHandler {
	public:
		ClientFileOpenMessageHandler(DV *dv, int socket, const std::vector<std::string> &params);

		virtual void serve() override;


	private:
		static constexpr int kFilenameIndex = 1;
		static constexpr int kAppIdIndex = 2;
		static constexpr int kJobParamsStartIndex = 3;
		static constexpr int kNeededVectorSize = 4;

		std::string filename_;
		dv::id_type appid_;
		std::vector<std::string> jobparams_;
	};

}

#endif //DV_SERVER_CLIENT_LISTENERS_CLIENTFILEOPENMESSAGEHANDLER_H_
