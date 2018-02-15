//
// Created by Pirmin Schmid on 19.09.17.
//

#ifndef DV_SERVER_SIMULATOR_LISTENERS_SIMULATORCHECKPOINTCREATEMESSAGEHANDLER_H_
#define DV_SERVER_SIMULATOR_LISTENERS_SIMULATORCHECKPOINTCREATEMESSAGEHANDLER_H_

#include <string>
#include <vector>

#include "../../DVBasicTypes.h"
#include "../MessageHandler.h"


/**
 * This handler is almost identical to the SimulatorFileCreateMessageHandler.
 * Small modifications:
 * - no cache modifications, of course
 * - lookup re existing file in the checkpoint/restart file tree
 * - future option: dynamically adjust this tree upon close message
 *   but not at the moment. note: DVLib is currently sending no close
 *   message for checkpoint files.
 */

namespace dv {

	class SimulatorCheckpointCreateMessageHandler : public MessageHandler {
	public:
		SimulatorCheckpointCreateMessageHandler(DV *dv, int socket, const std::vector<std::string> &params);

		virtual void serve() override;

	private:
		static constexpr int kJobIdIndex = 1;
		static constexpr int kFilenameIndex = 2;
		static constexpr int kNeededVectorSize = 3;

		dv::id_type jobid_;
		std::string filename_;
	};
}

#endif //DV_SERVER_SIMULATOR_LISTENERS_SIMULATORCHECKPOINTCREATEMESSAGEHANDLER_H_
