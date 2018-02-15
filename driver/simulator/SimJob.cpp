//
// Created by Pirmin Schmid on 14.04.17.
//


#include "SimJob.h"

#include <stdio.h>
#include <stdlib.h>

#include <iostream>

#include "../server/DV.h"
#include "Simulator.h"
#include "../caches/filecaches/FileDescriptor.h"
#include "../toolbox/FileSystemHelper.h"
#include "../toolbox/StringHelper.h"
#include "../toolbox/TimeHelper.h"

namespace dv {

	SimJob::SimJob(DV *dvl_ptr, dv::id_type appid, dv::id_type target_nr,
				   std::unique_ptr<toolbox::KeyValueStore> parameters) :
			dv_ptr_(dvl_ptr), appid_(appid),  parameters_(std::move(parameters)){

		//parameters_.fromString(parameters[0]);
		setSimStart(target_nr);
		parameters_->setString("dvl_address", dv_ptr_->getIpAddress());
		parameters_->setString("dvl_port", dv_ptr_->getSimPort());
        
        //FIXME: min_sim_target_ should be 0 if this SimJob has been prefetched!!!
        min_sim_target_ = target_nr;

        time_to_kill_ = dv_ptr_->getConfigPtr()->sim_kill_threshold_;
	}

	std::string SimJob::getParameter(const std::string &key) const {
		return parameters_->getString(key);
	}

	void SimJob::setParameter(const std::string &key, const std::string &value) {
		parameters_->setString(key, value);
	}

	const std::unordered_map<std::string, std::string> &SimJob::getParameters() const {
		return parameters_->getStoreMap();
	}

	void SimJob::extendParameters(const std::unordered_map<std::string, std::string> &additional_parameters) {
		parameters_->extendMap(additional_parameters);
	}

	void SimJob::printParameters(std::ostream *out) const {
		*out << "SimJob parameters:" << std::endl;
		for (const auto &item : parameters_->getStoreMap()) {
			*out << "   " << item.first << ": " << item.second << std::endl;
		}
		*out << std::endl;
	}

	bool SimJob::isValidJob() const {
		return valid_job_;
	}

	void SimJob::invalidateJob() {
		valid_job_ = false;
	}

	dv::id_type SimJob::getJobId() const {
		return jobid_;
	}

	void SimJob::setJobId(dv::id_type jobid) {
		jobid_ = jobid;
		parameters_->setInt<dv::id_type>("jobid", jobid);
	}

	dv::id_type SimJob::getGniRank() const {
		return gnirank_;
	}

	void SimJob::setGniRank(dv::id_type gnirank) {
		gnirank_ = gnirank;
		parameters_->setInt<dv::id_type>("gnirank", gnirank);
	}

	dv::id_type SimJob::getSimStart() const {
		return sim_start_nr_;
	}

    dv::id_type SimJob::getCurrentNr() const {
        if (last_nr_>=0) return last_nr_;
        return sim_start_nr_;
    }

	dv::id_type SimJob::getAppId() {
		return appid_;
	}

	void SimJob::setSimStart(dv::id_type start) {
		sim_start_nr_ = start;
		parameters_->setInt<dv::id_type>("simstart", start);
	}

	dv::id_type SimJob::getSimStop() const {
		return sim_stop_nr_;
	}

	void SimJob::setSimStop(dv::id_type stop) {
		sim_stop_nr_ = stop;
		parameters_->setInt<dv::id_type>("simstop", stop);
	}

    bool SimJob::hasToBeKilled(){
        //assert(time_to_kill>=0);

        /*
        //wait for the next restart interval
        std::cout << "SimJob:: hasToBeKilled: sim_kill_threshold_: " << dvl_ptr_->getConfigPtr()->sim_kill_threshold_ << "; time_to_kill_: " << time_to_kill_ << "; kill_after_this_restart_interval_: " << kill_after_this_restart_interval_ << "; last_accessed_restart_interval_: " << last_accessed_restart_interval_ << std::endl;
        return dvl_ptr_->getConfigPtr()->sim_kill_threshold_ > 0 && time_to_kill_<=0 && kill_after_this_restart_interval_!=last_accessed_restart_interval_;
        */
    
        //std::cout << "SimJob:: hasToBeKilled: sim_kill_threshold_: " << dvl_ptr_->getConfigPtr()->sim_kill_threshold_ << "; time_to_kill_: " << time_to_kill_ << std::endl;
        return dv_ptr_->getConfigPtr()->sim_kill_threshold_ > 0 && time_to_kill_<=0;


    }
    
    void SimJob::handleClientFileOpen(dv::id_type nr){
        time_to_kill_ = dv_ptr_->getConfigPtr()->sim_kill_threshold_;


        //This is a bit of an overkill: another simulation can serve the same request earlier, so
        //it may be not needed for this specific instance to reach min_sim_target.
        min_sim_target_ = std::max(min_sim_target_, nr);


        //std::cout << "SimJob:: resetting time_to_kill_: " << time_to_kill_ << std::endl;
    }

    void SimJob::handleSimulatorFileCreate(const std::string &name, FileDescriptor *cache_entry) {
        last_nr_ = dv_ptr_->getSimulatorPtr()->result2nr(name);

        //we decrease time_to_kill_ only if we already simulated what we know we will need for sure
        if (time_to_kill_>0 && min_sim_target_<last_nr_) time_to_kill_--;

        //last_accessed_restart_interval_ = dvl_ptr_->getSimulatorPtr()->getCheckpointNr(last_nr_);


        //if (time_to_kill_==0){
        //    kill_after_this_restart_interval_ = last_accessed_restart_interval_;
        //}

    }


    void SimJob::handleSimulatorFileClose(const std::string &name, FileDescriptor *cache_entry) {
		toolbox::TimeHelper::time_point_type now = toolbox::TimeHelper::now();
		if (files_ == 0) {
			setup_duration_ = toolbox::TimeHelper::seconds(start_time_, now);
		} else {
			taus_.push_back(toolbox::TimeHelper::seconds(last_time_, now));
		}
		last_time_ = now;


		produced_file_numbers_.emplace(last_nr_);
		++files_;
		if (cache_entry->isFilePrefetched()) {
			is_prefetched_ = true;
		}
	}

	void SimJob::terminate() {
		Simulator *simulator_ptr = dv_ptr_->getSimulatorPtr();
		simulator_ptr->incFilesCount(files_);
		simulator_ptr->incSimulationsCount();
		removeRedirectFolders();
	}

	double SimJob::getSetupDuration() const {
		return setup_duration_;
	}

	const std::vector<double> &SimJob::getTaus() const {
		return taus_;
	}

	void SimJob::setPrefetched(bool prefetched) {
		is_prefetched_ = prefetched;
	}

	bool SimJob::isPrefetched() const {
		return is_prefetched_;
	}

	void SimJob::prepare() {
		std::string appid_str = std::to_string(appid_);
		std::string uid_str = dv_ptr_->getDvlJobid();
		std::string job_id_str = parameters_->getString("jobid");

		// call Lua script for final adjustments
		std::string p = parameters_->toString();
		std::string adjustments = dv_ptr_->getConfigPtr()->simjob_final_adjustments(p, sim_start_nr_, sim_stop_nr_);
		parameters_->fromString(adjustments);

		// note: we *must not* set sim_start_nr_ or sim_stop_nr_ to the adjusted values
		// they will be needed for the willProduce() lookup
		// however, adjusting the strings in parameters_ is fine
		// (see handling above after calling the Lua script)

		if (dv_ptr_->getConfigPtr()->dv_debug_output_on_) {
			/*std::cout << "SimJob::prepare(): adjusted values for job generation in simulator specific number space:"
					  << " simstart from " << sim_start_nr_ << " to " << parameters_->getString("simstart")
					  << ", simstop from " << sim_stop_nr_ << " to " << parameters_->getString("simstop")
					  << std::endl;
            */
            std::cout << "SimJob::prepare: " 
                      << parameters_->getString("simstart") 
                      << " " << parameters_->getString("simstop") 
                      << " " << parameters_->getDouble("simstop") - parameters_->getDouble("simstart") << std::endl;
		}

		// additional variables available for substitution
		parameters_->setString("__APPID__", appid_str);
		parameters_->setString("__UID__", uid_str);
		parameters_->setString("__JOBID__", job_id_str);
		parameters_->setString("__ID__", "_" + uid_str + "_" + appid_str + "_" + job_id_str);

		// make file names available for substitution, too
		std::string par_out_name = dv_ptr_->getConfigPtr()->sim_parameter_output_file_;
		if (!par_out_name.empty()) {
			toolbox::StringHelper::replaceAllStringsInPlace(&par_out_name, "APPID", appid_str);
			toolbox::StringHelper::replaceAllStringsInPlace(&par_out_name, "UID", uid_str);
			toolbox::StringHelper::replaceAllStringsInPlace(&par_out_name, "JOBID", job_id_str);
		}
		parameters_->setString("__PARFILE__", par_out_name);

		std::string job_out_name = dv_ptr_->getConfigPtr()->sim_job_output_file_;
		toolbox::StringHelper::replaceAllStringsInPlace(&job_out_name, "APPID", appid_str);
		toolbox::StringHelper::replaceAllStringsInPlace(&job_out_name, "UID", uid_str);
		toolbox::StringHelper::replaceAllStringsInPlace(&job_out_name, "JOBID", job_id_str);
		parameters_->setString("__JOBFILE__", job_out_name);

		// optional parameter template file
		if(!par_out_name.empty()) {
			toolbox::TextTemplate par_template(dv_ptr_->getConfigPtr()->sim_parameter_template_style_);
			if (!par_template.readFile(dv_ptr_->getConfigPtr()->sim_parameter_template_file_)) {
				std::cerr << "Error in SimJob: could not read parameter template file "
						  << dv_ptr_->getConfigPtr()->sim_parameter_template_file_ << std::endl;
				return;
			}
			par_template.substituteWith(*parameters_);
			std::string par_out_fullname = toolbox::StringHelper::joinPath(dv_ptr_->getConfigPtr()->sim_config_path_,
																		   par_out_name);
			if (!par_template.writeFile(par_out_fullname)) {
				std::cerr << "Error in SimJob: could not write simulation parameter file "
						  << par_out_fullname << std::endl;
				return;
			}
			if (dv_ptr_->getConfigPtr()->dv_debug_output_on_) {
				std::cout << "parameter file " << par_out_name << " written to " << par_out_fullname << std::endl;
			}
		}

		// job file
		toolbox::TextTemplate job_template(dv_ptr_->getConfigPtr()->sim_job_template_style_);
		if (!job_template.readFile(dv_ptr_->getConfigPtr()->sim_job_template_file_)) {
			std::cerr << "Error in SimJob: could not read job template file "
					  << dv_ptr_->getConfigPtr()->sim_job_template_file_ << std::endl;
			return;
		}
		job_template.substituteWith(*parameters_);
		std::string job_out_fullname = toolbox::StringHelper::joinPath(dv_ptr_->getConfigPtr()->sim_config_path_,
																	   job_out_name);
		if (!job_template.writeFile(job_out_fullname)) {
			std::cerr << "Error in SimJob: could not write simulation job file " << job_out_fullname << std::endl;
			return;
		}
		if (dv_ptr_->getConfigPtr()->dv_debug_output_on_) {
			std::cout << "job script file " << job_out_name << " written to " << job_out_fullname << std::endl;
		}
		jobname_ = job_out_fullname;
	}

	bool SimJob::willProduceFile(const std::string &filename) const {
		dv::id_type type = dv_ptr_->getSimulatorPtr()->getResultFileType(filename);
		if (type == 0) {
			return false;
		}

		return willProduceNr(dv_ptr_->getSimulatorPtr()->result2nr(filename, type));
	}

	bool SimJob::willProduceNr(dv::id_type nr) const {
		auto it = produced_file_numbers_.find(nr);
		if (it != produced_file_numbers_.end()) {
			return false;
		}

		return valid_job_ && sim_start_nr_ <= nr && nr <= sim_stop_nr_;
	}

	bool SimJob::fileIsInSimulationRange(const std::string &filename) const {
		dv::id_type type = dv_ptr_->getSimulatorPtr()->getResultFileType(filename);
		if (type == 0) {
			return false;
		}

		return nrIsInSimulationRange(dv_ptr_->getSimulatorPtr()->result2nr(filename, type));
	}

	bool SimJob::nrIsInSimulationRange(dv::id_type nr) const {
		return valid_job_ && sim_start_nr_ <= nr && nr <= sim_stop_nr_;
	}

	dv::id_type SimJob::launch() {
		prepare();
		std::cout << "starting job " << jobname_ << std::endl;

		if (!createRedirectFolders()) {
			std::cerr << "SEVERE ERROR: could not create redirect folder" << std::endl;
			return -1;
		}

		std::string command = "bash " + jobname_;
		std::string result;
		FILE *in;
		char buffer[kShellBufferSize];
		in = popen(command.c_str(), "r");
		if (in == nullptr) {
			std::cerr << "SEVERE ERROR: could not launch the job using bash" << std::endl;
			return -1;
		}

		while (fgets(buffer, sizeof(buffer), in) != nullptr) {
			std::string next = buffer;
			result += next;
		}
		pclose(in);

		try {
			sysjobid_ = std::stol(result);
		} catch (const std::invalid_argument &ia) {
			std::cerr << "SEVERE ERROR: could not catch sysjobid from string " << result << std::endl;
			return -1;
		}
		std::cout << "simulation sysjobid: " << sysjobid_ << std::endl;

        start_time_ = toolbox::TimeHelper::now();
		return jobid_;
	}

	const std::string &SimJob::getRedirectPath_result() const {
		return redirect_path_result_;
	}

	const std::string &SimJob::getRedirectPath_checkpoint() const {
		return redirect_path_checkpoint_;
	}

	bool SimJob::createRedirectFolders() {
		redirect_path_result_ = toolbox::StringHelper::joinPath(dv_ptr_->getRedirectPath(), "simjob_" + std::to_string(jobid_));
		redirect_path_checkpoint_ = toolbox::StringHelper::joinPath(redirect_path_result_, "_DV_chk_");
		std::cout << "Creating redirect folders for results " << redirect_path_result_
				  << " and checkpoints " << redirect_path_checkpoint_ << std::endl;
		std::string command = "mkdir -p " + redirect_path_checkpoint_; // creates both
		system(command.c_str());

		return toolbox::FileSystemHelper::folderExists(redirect_path_checkpoint_); // checks both
	}

	void SimJob::removeRedirectFolders() {
		if(!redirect_path_result_.empty() && toolbox::FileSystemHelper::folderExists(redirect_path_result_)) {
			std::cout << std::endl << "Removing redirect folder " << redirect_path_result_ << std::endl;
			std::string command = "rm -r " + redirect_path_result_; // removes both
			system(command.c_str());
		}
	}
}
