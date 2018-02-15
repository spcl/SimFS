//
// Created by Pirmin Schmid on 16.04.17.
//

#ifndef DV_SERVER_SIMULATOR_LISTENERS_SIMULATORVARIABLEPUTMESSAGEHANDLER_H_
#define DV_SERVER_SIMULATOR_LISTENERS_SIMULATORVARIABLEPUTMESSAGEHANDLER_H_

#include <string>
#include <vector>

#include "../../DVBasicTypes.h"
#include "../MessageHandler.h"

namespace dv {

	class SimulatorVariablePutMessageHandler : public MessageHandler {
	public:
		SimulatorVariablePutMessageHandler(DV *dv, int socket, const std::vector<std::string> &params);

		virtual void serve() override;


	private:
		static constexpr int kJobIdIndex = 1;
		static constexpr int kFilenameIndex = 3; // ! switched
		static constexpr int kVarIdIndex = 2;
		static constexpr int kDimensionsIndex = 4;
		static constexpr int kOffsetsIndex = 5;
		static constexpr int kCountsIndex = 6;
		static constexpr int kNeededVectorSize = 5;
		static constexpr int kNeededVectorSizeWithOffsets = 7;

		dv::id_type jobid_;
		std::string filename_;
	};

}


#endif //DV_SERVER_SIMULATOR_LISTENERS_SIMULATORVARIABLEPUTMESSAGEHANDLER_H_
