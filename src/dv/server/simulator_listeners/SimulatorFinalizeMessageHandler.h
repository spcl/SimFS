//
// Created by Pirmin Schmid on 16.04.17.
//

#ifndef DV_SERVER_SIMULATOR_LISTENERS_SIMULATORFINALIZEMESSAGEHANDLER_H_
#define DV_SERVER_SIMULATOR_LISTENERS_SIMULATORFINALIZEMESSAGEHANDLER_H_

#include <string>
#include <vector>

#include "../../DVBasicTypes.h"
#include "../MessageHandler.h"


namespace dv {

	class SimulatorFinalizeMessageHandler : public MessageHandler {
	public:
		SimulatorFinalizeMessageHandler(DV *dv, int socket, const std::vector<std::string> &params);

		virtual void serve() override;


	private:
		static constexpr int kJobIdIndex = 1;
		static constexpr int kNeededVectorSize = 2;

		dv::id_type jobid_;
	};

}


#endif //DV_SERVER_SIMULATOR_LISTENERS_SIMULATORFINALIZEMESSAGEHANDLER_H_
