//
// Created by Pirmin Schmid on 15.04.17.
//


#include "DVConfig.h"

#include <iostream>

namespace dv {

	//--- initialization -------------------------------------------------------

	bool DVConfig::loadConfigFile(const std::string &filename) {
		int r = lw_.loadFile(filename);
		if (!r) {
			std::cerr << "Could not load config script " << filename
					  << ". File available? Lua syntax correct?" << std::endl;
			std::cerr << "Lua error message: " << lw_.getErrorString() << std::endl;
			return false;
		}

		return checkApi();
	}

	bool DVConfig::init() {
		if (lw_.callInt2Int("init", kApiVersion) != 1) {
			return false;
		}

		return fetchConstants();
	}

	bool DVConfig::run_checks() {
		return lw_.callInt2Int("run_checks", kApiVersion) == 1;
	}

	/**
	 * checks validity of config arguments as far as deemed necessary
	 * also performs some internal calculations
	 */
	bool DVConfig::assure_config_ok() {
		// internal configuration
		dv_max_prefetching_intervals_ = dv_max_horizontal_prefetching_intervals_ * dv_max_vertical_prefetching_intervals_;
		if (dv_max_prefetching_intervals_ != 0) {
			// 0 == off takes precedence
			// otherwise, check for unlimited
			if (dv_max_horizontal_prefetching_intervals_ < 0) {
				dv_max_horizontal_prefetching_intervals_ = -1; // assure value
				dv_max_prefetching_intervals_ = -1;
			} else if (dv_max_vertical_prefetching_intervals_ < 0) {
				dv_max_vertical_prefetching_intervals_ = -1; // assure value
				dv_max_prefetching_intervals_ = -1;
			}
		}

		// checks
		if (filecache_size_ <= 0) {
			std::cerr << "filecache_size must be > 0." << std::endl;
			return false;
		}

		if (filecache_fifo_queue_size_ < 0) {
			std::cerr << "filecache_fifo_queue_size_ must be >= 0." << std::endl;
			return false;
		}

		if (filecache_fifo_queue_size_ >= filecache_size_) {
			std::cerr << "filecache_fifo_queue_size_ must be < filecache_size." << std::endl;
			return false;
		}

		filecache_embedded_cache_size_ = filecache_size_ - filecache_fifo_queue_size_;

		if (FileCache::getFileCacheType(filecache_type_) == FileCache::kLIRS) {
			if (filecache_lir_set_size_ <= 0) {
				std::cerr << "LIRS cache: LIR set size must be > 0." << std::endl;
				return false;
			}

			if (filecache_lir_set_size_ >= filecache_embedded_cache_size_) {
				std::cerr << "LIRS cache: LIR set size must be < effective/embedded cache size." << std::endl;
				return false;
			}
		}

		if (filecache_protected_mrus_ <= 0) {
			std::cerr << "filecache_protected_mrus must be > 0." << std::endl;
			return false;
		}

		if (filecache_protected_mrus_ > filecache_embedded_cache_size_) {
			std::cerr << "filecache_protected_mrus must be <= effective/embedded cache size." << std::endl;
			return false;
		}

		if (filecache_penalty_factor_ < 0.0 || 1.0 < filecache_penalty_factor_) {
			std::cerr << "filecache_penalty_factor must be in [0.0, 1.0]." << std::endl;
			return false;
		}

		if (sim_config_path_.empty()
			|| sim_checkpoint_path_.empty()
			|| sim_result_path_.empty()) {
			std::cerr << "All paths must be set: config, checkpoint, result" << std::endl;
			return false;
		}

		// TODO: implement more checks

		config_ok_ = true;
		return true;
	}

	void DVConfig::print(std::ostream *out) {
		if (!config_ok_) {
			if (!assure_config_ok()) {
				*out << std::endl << "!!! The following configuration did not pass the validation check !!!" << std::endl;
			}
		}

		*out << std::endl
			 << "Configuration settings" << std::endl
			 << "dv_hostname = " << dv_hostname_ << std::endl
			 << "dv_sim_port = " << dv_sim_port_ << std::endl
			 << "dv_client_port = " << dv_client_port_ << std::endl
			 << "dv_max_parallel_simjobs = " << dv_max_parallel_simjobs_ << std::endl
			 << "dv_max_horizontal_prefetching_intervals = " << dv_max_horizontal_prefetching_intervals_ << std::endl
			 << "dv_max_vertical_prefetching_intervals = " << dv_max_vertical_prefetching_intervals_ << std::endl
			 << "=> derived: dv_max_prefetching_intervals = " << dv_max_prefetching_intervals_
			 << (dv_max_prefetching_intervals_ == 0 ? " (prefetching off)" :
				 (dv_max_prefetching_intervals_ == -1 ? " (prefetching unlimited)" : "")) << std::endl
			 << "dv_batch_job_id = " << dv_batch_job_id_ << std::endl
			 << "dv_stat_label = " << dv_stat_label_ << std::endl;

		if (optional_dv_prefetch_all_files_at_once_) {
			*out << "optional_dv_prefetch_all_files_at_once = on; with optional_sim_max_nr = "
				 << optional_sim_max_nr_ << std::endl;
		} else {
			*out << "optional_dv_prefetch_all_files_at_once = off" << std::endl;
		}

		*out << "sim_config_path = " << sim_config_path_ << std::endl
			 << "sim_checkpoint_path = " << sim_checkpoint_path_ << std::endl
			 << "sim_result_path = " << sim_result_path_ << std::endl
			 << "sim_temporary_redirect_path = " << sim_temporary_redirect_path_ << std::endl
			 << "sim_parameter_template_style = " << (sim_parameter_template_style_ + 1) << std::endl
			 << "sim_parameter_template_file = " << sim_parameter_template_file_ << std::endl
			 << "sim_parameter_output_file = " << sim_parameter_output_file_ << std::endl
			 << "sim_job_template_style  = " << (sim_job_template_style_ + 1) << std::endl
			 << "sim_job_template_file = " << sim_job_template_file_ << std::endl
			 << "sim_job_output_file = " << sim_job_output_file_ << std::endl
			 << "sim_kill_threshold = " << sim_kill_threshold_ << std::endl;

		*out << "filecache_type = " << filecache_type_ << std::endl
			 << "filecache_size = " << filecache_size_ << std::endl
			 << "filecache_fifo_queue_size = " << filecache_fifo_queue_size_ << std::endl
			 << (0 < filecache_fifo_queue_size_? "=> FIFO queue wrapper active: embedded cache size "
												 + std::to_string(filecache_embedded_cache_size_)
												 + "\n" : "")
			 << "filecache_lir_set_size = " << filecache_lir_set_size_ << std::endl
			 << "filecache_protected_mrus = " << filecache_protected_mrus_ << std::endl
			 << "filecache_penalty_factor = " << filecache_penalty_factor_ << std::endl
			 << std::endl;
	}

	//--- functions ------------------------------------------------------------

	dv::id_type DVConfig::get_checkpoint_file_type(const std::string &filename) {
		return lw_.callString2Int("get_checkpoint_file_type", filename);
	}

	dv::id_type DVConfig::checkpoint2nr(const std::string &filename, dv::id_type checkpoint_file_type) {
		if (checkpoint_file_type == 0) {
			checkpoint_file_type = get_checkpoint_file_type(filename);
		}

		if (checkpoint_file_type == 0) {
			std::cerr << "File type of file " << filename << " not recognized." << std::endl;
			return 0;
		}

		return lw_.callStringInt2Int("checkpoint2nr", filename, checkpoint_file_type);
	}

	dv::id_type DVConfig::get_result_file_type(const std::string &filename) {
		return lw_.callString2Int("get_result_file_type", filename);
	}

	dv::id_type DVConfig::result2nr(const std::string &filename, dv::id_type result_file_type) {
		if (result_file_type == 0) {
			result_file_type = get_result_file_type(filename);
		}

		if (result_file_type == 0) {
			std::cerr << "File type of file " << filename << " not recognized." << std::endl;
			return 0;
		}

		return lw_.callStringInt2Int("result2nr", filename, result_file_type);
	}

	std::string
	DVConfig::simjob_final_adjustments(const std::string &map_string, dv::id_type simstart, dv::id_type simstop) {
		return lw_.callStringIntInt2String("simjob_final_adjustments", map_string, simstart, simstop);
	}

	//--- private helper methods -----------------------------------------------

	bool DVConfig::checkApi() {
		lua::LuaWrapper::int_type version = lw_.getInt("api_version");
		if (version != kApiVersion) {
			std::cerr << "Config script API version mismatch: expected " << kApiVersion
					  << ", found " << version << std::endl;
			return false;
		}

		// API checks: functions (note: signature is not checked)
		checkApiStatus_ = true;
		checkApiPart(lua::LuaWrapper::kFunction, "init");
		checkApiPart(lua::LuaWrapper::kFunction, "run_checks");
		checkApiPart(lua::LuaWrapper::kFunction, "get_checkpoint_file_type");
		checkApiPart(lua::LuaWrapper::kFunction, "checkpoint2nr");
		checkApiPart(lua::LuaWrapper::kFunction, "get_result_file_type");
		checkApiPart(lua::LuaWrapper::kFunction, "result2nr");
		checkApiPart(lua::LuaWrapper::kFunction, "simjob_final_adjustments");

		// API checks: dv constants
		checkApiPart(lua::LuaWrapper::kString, "dv_hostname");
		checkApiPart(lua::LuaWrapper::kString, "dv_client_port");
		checkApiPart(lua::LuaWrapper::kString, "dv_sim_port");
		checkApiPart(lua::LuaWrapper::kString, "dv_batch_job_id");
		checkApiPart(lua::LuaWrapper::kString, "dv_statistics_label");

		checkApiPart(lua::LuaWrapper::kInt, "dv_max_parallel_simjobs");
		checkApiPart(lua::LuaWrapper::kInt, "dv_max_horizontal_prefetching_intervals");
		checkApiPart(lua::LuaWrapper::kInt, "dv_max_vertical_prefetching_intervals");
		// note: dv_max_prefetching_intervals is derived and not read from config file
		checkApiPart(lua::LuaWrapper::kInt, "optional_dv_prefetch_all_files_at_once");

		// API checks: simulator constants
		checkApiPart(lua::LuaWrapper::kString, "sim_config_path");
		checkApiPart(lua::LuaWrapper::kString, "sim_checkpoint_path");
		checkApiPart(lua::LuaWrapper::kString, "sim_result_path");
		checkApiPart(lua::LuaWrapper::kString, "sim_temporary_redirect_path");

		checkApiPart(lua::LuaWrapper::kInt, "sim_parameter_template_style");
		checkApiPart(lua::LuaWrapper::kString, "sim_parameter_template_file");
		checkApiPart(lua::LuaWrapper::kString, "sim_parameter_output_file");

		checkApiPart(lua::LuaWrapper::kInt, "sim_job_template_style");
		checkApiPart(lua::LuaWrapper::kString, "sim_job_template_file");
		checkApiPart(lua::LuaWrapper::kString, "sim_job_output_file");

		checkApiPart(lua::LuaWrapper::kInt, "sim_kill_threshold");

		checkApiPart(lua::LuaWrapper::kInt, "optional_sim_max_nr");
		checkApiPart(lua::LuaWrapper::kInt, "optional_dv_prefetch_all_files_at_once");

		// API checks: filecache constants
		checkApiPart(lua::LuaWrapper::kString, "filecache_type");
		checkApiPart(lua::LuaWrapper::kInt, "filecache_size");
		checkApiPart(lua::LuaWrapper::kInt, "filecache_fifo_queue_size");
		checkApiPart(lua::LuaWrapper::kInt, "filecache_lir_set_size");
		checkApiPart(lua::LuaWrapper::kInt, "filecache_protected_mrus");
		checkApiPart(lua::LuaWrapper::kDouble, "filecache_penalty_factor");

		return checkApiStatus_;
	}

	void DVConfig::checkApiPart(lua::LuaWrapper::ApiType expected_type, const std::string &name) {
		if (!lw_.check(expected_type, name)) {
			std::string type;
			switch(expected_type) {
				case lua::LuaWrapper::kFunction:
					type = "function";
					break;
				case lua::LuaWrapper::kString:
					type = "string";
					break;
				case lua::LuaWrapper::kInt:
					type = "integer";
					break;
				case lua::LuaWrapper::kDouble:
					type = "floating point";
					break;
			}

			std::cerr << "config file: missing or wrong type: " << name << " (expected: " << type << ")" << std::endl;
			checkApiStatus_ = false;
		}
	}

	bool DVConfig::fetchConstants() {
		// DV
		dv_hostname_ = lw_.getString("dv_hostname");
		dv_client_port_ = lw_.getString("dv_client_port");
		dv_sim_port_ = lw_.getString("dv_sim_port");
		dv_batch_job_id_ = lw_.getString("dv_batch_job_id");
		dv_stat_label_ = lw_.getString("dv_statistics_label");

		dv_max_parallel_simjobs_ = lw_.getInt("dv_max_parallel_simjobs");
		dv_max_horizontal_prefetching_intervals_ = lw_.getInt("dv_max_horizontal_prefetching_intervals");
		dv_max_vertical_prefetching_intervals_ = lw_.getInt("dv_max_vertical_prefetching_intervals");

        
		// multiplication and additional checks happen during assure_config_ok()

		optional_dv_prefetch_all_files_at_once_ = lw_.getDouble("optional_dv_prefetch_all_files_at_once") == 1;

		// simulator

		sim_config_path_ = lw_.getString("sim_config_path");
		sim_checkpoint_path_ = lw_.getString("sim_checkpoint_path");
		sim_result_path_ = lw_.getString("sim_result_path");
		sim_temporary_redirect_path_ = lw_.getString("sim_temporary_redirect_path");

        sim_kill_threshold_ = lw_.getInt("sim_kill_threshold");

		sim_parameter_template_file_ = lw_.getString("sim_parameter_template_file");
		switch(lw_.getInt("sim_parameter_template_style")) {
			case 1:
				sim_parameter_template_style_ = toolbox::TextTemplate::kDoubleBracketsStyle;
				break;
			case 2:
				sim_parameter_template_style_ = toolbox::TextTemplate::kDollarSignStyle;
				break;
			default:
				sim_parameter_template_file_ = "";
		}
		sim_parameter_output_file_ = lw_.getString("sim_parameter_output_file");


		sim_job_template_file_ = lw_.getString("sim_job_template_file");
		switch(lw_.getInt("sim_job_template_style")) {
			case 1:
				sim_job_template_style_ = toolbox::TextTemplate::kDoubleBracketsStyle;
				break;
			case 2:
				sim_job_template_style_ = toolbox::TextTemplate::kDollarSignStyle;
				break;
			default:
				sim_job_template_file_ = "";
		}
		sim_job_output_file_ = lw_.getString("sim_job_output_file");

		optional_sim_max_nr_ = lw_.getInt("optional_sim_max_nr");


		// filecache
		filecache_type_ = lw_.getString("filecache_type");
		filecache_size_ = lw_.getInt("filecache_size");
		filecache_fifo_queue_size_ = lw_.getInt("filecache_fifo_queue_size");
		filecache_lir_set_size_ = lw_.getInt("filecache_lir_set_size");
		filecache_protected_mrus_ = lw_.getInt("filecache_protected_mrus");
		filecache_penalty_factor_ = lw_.getDouble("filecache_penalty_factor");

		return true;
	}
}
