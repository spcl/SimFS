//
// Created by Pirmin Schmid on 16.04.17.
//

#ifndef DV_SERVER_CLIENT_LISTENERS_CLIENTFILECLOSEMESSAGEHANDLER_H_
#define DV_SERVER_CLIENT_LISTENERS_CLIENTFILECLOSEMESSAGEHANDLER_H_

#include <string>
#include <vector>

#include "../MessageHandler.h"

namespace dv {

	class ClientFileCloseMessageHandler : public MessageHandler {
	public:
		ClientFileCloseMessageHandler(DV *dv, int socket, const std::vector<std::string> &params);

		virtual void serve() override;


	private:
		static constexpr int kFilenameIndex = 1;
		static constexpr int kNeededVectorSize = 2;

		std::string filename_;
	};

}

#endif //DV_SERVER_CLIENT_LISTENERS_CLIENTFILECLOSEMESSAGEHANDLER_H
