//
// Created by Pirmin Schmid on 16.04.17.
//

#ifndef DV_SERVER_CLIENT_LISTENERS_CLIENTVARIABLEGETMESSAGEHANDLER_H
#define DV_SERVER_CLIENT_LISTENERS_CLIENTVARIABLEGETMESSAGEHANDLER_H

#include <string>
#include <vector>

#include "../../DVBasicTypes.h"
#include "../MessageHandler.h"


namespace dv {

	class ClientVariableGetMessageHandler : public MessageHandler {
	public:
		ClientVariableGetMessageHandler(DV *dv, int socket, const std::vector<std::string> &params);

		virtual void serve() override;


	private:
		static constexpr int kFilenameIndex = 1;
		static constexpr int kVariableIdIndex = 2;
		static constexpr int kAppIdIndex = 3;
		static constexpr int kNeededVectorSize = 4;
		static constexpr bool kDimensionDetailsOn = false;
		static constexpr int kDimensionsIndex = 4;
		static constexpr int kOffsetsIndex = 5;
		static constexpr int kCountsIndex = 6;
		static constexpr int kNeededVectorSizeWithDetails = 7;

		std::string filename_;
		std::string var_id_;
		dv::id_type appid_;

		dv::dimension_type dimensions_;
		std::vector<dv::offset_type> offsets_;
		std::vector<dv::offset_type> counts_;
	};

}

#endif //DV_SERVER_CLIENT_LISTENERS_CLIENTVARIABLEGETMESSAGEHANDLER_H
