//
// Created by Pirmin Schmid on 23.07.17.
//

#ifndef DV_SERVER_COMMON_LISTENERS_EXTENDEDAPIMESSAGEHANDLER_H_
#define DV_SERVER_COMMON_LISTENERS_EXTENDEDAPIMESSAGEHANDLER_H_

#include <string>
#include <vector>

#include "../../DVBasicTypes.h"
#include "../MessageHandler.h"

namespace dv {


	/**
	 * It must be assured that the message format and relevant API constants
	 * match their counterparts in the clientlib implementation.
	 *
	 * message format:
 	 * E:appid:function_nr:n_arguments:argument0:argument1:argument2:...
 	 *
	 */
	class ExtendedApiMessageHandler : public MessageHandler {
	public:
		ExtendedApiMessageHandler(DV *dv, int socket, const std::vector<std::string> &params);

		virtual void serve() override;


	private:
		static constexpr int kAppIdIndex = 1;
		static constexpr int kApiFunctionIndex = 2;
		static constexpr int kApiArgumentsCountIndex = 3;
		static constexpr int kApiFirstArgumentIndex = 4;
		static constexpr int kNeededVectorSize = 4;

		static constexpr int kHandleSetInfo = 1;
		static constexpr int kHandleGetInfo = 2;
		static constexpr int kHandleRequestRange = 3;
		static constexpr int kHandleTestFile = 4;
		static constexpr int kHandleStatus = 5;

		dv::id_type appid_;
		dv::id_type api_function_;
		dv::id_type api_arguments_count_;

		std::vector<std::string> api_arguments_;

		void handle_set_info();
		void handle_get_info();
		void handle_request_range();
		void handle_test_file();
		void handle_status();
	};

}

#endif //DV_SERVER_COMMON_LISTENERS_EXTENDEDAPIMESSAGEHANDLER_H_
