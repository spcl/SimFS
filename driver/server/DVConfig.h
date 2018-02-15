//
// Created by Pirmin Schmid on 15.04.17.
//

#ifndef DV_SERVER_DVCONFIG_H_
#define DV_SERVER_DVCONFIG_H_


#include <functional>
#include <ostream>
#include <string>

#include "../DVBasicTypes.h"
#include "../toolbox/LuaWrapper.h"
#include "../toolbox/TextTemplate.h"

#include "../caches/filecaches/FileCacheLRU.h"
#include "../caches/filecaches/FileCacheLIRS.h"

namespace dv {

	class DVConfig {
	public:
		static constexpr int kApiVersion = 5;

		// API versions
		// 0: initial version
		// 1: modification to horizontal and vertical prefetching intervals instead of global interval
		//    see also new command line arguments
		// 2: added LIR set size for LIRS cache
		// 3: added sim_kill_threshold
		// 4: added temporary redirect dummy path
		// 5: added filecache_fifo_queue_size

		//--- initialization ---------------------------------------------------
		bool loadConfigFile(const std::string &filename);
		bool init();

		/**
		 * is not called from DV server but from helper program for checking
		 * the validity of the actual config script.
		 */
		bool run_checks();


		/**
		 * called by server after handling all potential input sources:
		 * config file, env variables, command line arguments.
		 * Here is also the place to add additional calculations to modify
		 * internal settings.
		 */
		bool assure_config_ok();

		void print(std::ostream *out);

		//--- DV server --------------------------------------------------------
		bool dv_debug_output_on_;
		std::string dv_hostname_;
		std::string dv_sim_port_;
		std::string dv_client_port_;

		dv::id_type dv_max_parallel_simjobs_;
		dv::id_type dv_max_horizontal_prefetching_intervals_;
		dv::id_type dv_max_vertical_prefetching_intervals_;
		dv::id_type dv_max_prefetching_intervals_; /** this is the product of horizontal * vertical */

		std::string dv_batch_job_id_;
		std::string dv_stat_label_;

		bool optional_dv_prefetch_all_files_at_once_;


		//--- simulator --------------------------------------------------------
		bool sim_debug_output_on_;

		std::string sim_config_path_;
		std::string sim_checkpoint_path_;
		std::string sim_result_path_;
		std::string sim_temporary_redirect_path_;

		toolbox::TextTemplate::Style sim_parameter_template_style_;
		std::string sim_parameter_template_file_;
		std::string sim_parameter_output_file_;

		toolbox::TextTemplate::Style sim_job_template_style_;
		std::string sim_job_template_file_;
		std::string sim_job_output_file_;

		dv::id_type optional_sim_max_nr_;
        
        dv::id_type sim_kill_threshold_;

		//--- filecache --------------------------------------------------------
		bool filecache_debug_output_on_;
		bool filecache_details_debug_output_on_;

		std::string filecache_type_;
		FileCacheLRU::ID_type filecache_size_;
		FileCacheLRU::ID_type filecache_fifo_queue_size_;
		FileCacheLRU::ID_type filecache_embedded_cache_size_; /** calculated during assure routine */
		FileCacheLIRS::ID_type filecache_lir_set_size_;
		FileCacheLRU::ID_type filecache_protected_mrus_;
		double filecache_penalty_factor_;


		//--- functions --------------------------------------------------------

		dv::id_type get_checkpoint_file_type(const std::string &filename);
		dv::id_type checkpoint2nr(const std::string &filename, dv::id_type checkpoint_file_type);

		dv::id_type get_result_file_type(const std::string &filename);
		dv::id_type result2nr(const std::string &filename, dv::id_type result_file_type);

		std::string simjob_final_adjustments(const std::string &map_string,
											 dv::id_type simstart, dv::id_type simstop);

		//--- miscellaneous ----------------------------------------------------
		std::function<bool()> stop_requested_predicate_;

	private:
		lua::LuaWrapper lw_;

		bool config_ok_ = false;

		bool checkApiStatus_;
		bool checkApi();
		void checkApiPart(lua::LuaWrapper::ApiType expected_type, const std::string &name);

		bool fetchConstants();

	};

}

#endif //DV_SERVER_DVCONFIG_H_
