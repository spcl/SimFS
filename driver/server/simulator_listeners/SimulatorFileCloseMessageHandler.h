//
// Created by Pirmin Schmid on 16.04.17.
//

#ifndef DV_SERVER_SIMULATOR_LISTENERS_SIMULATORFILECLOSEMESSAGEHANDLER_H_
#define DV_SERVER_SIMULATOR_LISTENERS_SIMULATORFILECLOSEMESSAGEHANDLER_H_


#include <string>
#include <vector>

#include "../../DVBasicTypes.h"
#include "../MessageHandler.h"

namespace dv {

	class SimulatorFileCloseMessageHandler : public MessageHandler {
	public:
		SimulatorFileCloseMessageHandler(DV *dv, int socket, const std::vector<std::string> &params);

		virtual void serve() override;


	private:
		static constexpr int kJobIdIndex = 1;
		static constexpr int kFilenameIndex = 2;
		static constexpr int kFilesizeIndex = 3;
		static constexpr int kNeededVectorSize = 4;

		dv::id_type jobid_;
		std::string filename_;
		dv::size_type filesize_;
	};

}

#endif //DV_SERVER_SIMULATOR_LISTENERS_SIMULATORFILECLOSEMESSAGEHANDLER_H_
