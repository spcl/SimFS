//
// Created by Pirmin Schmid on 14.04.17.
//

#ifndef DV_JOBS_SIMJOB_H_
#define DV_JOBS_SIMJOB_H_


#include <ostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../DVBasicTypes.h"
#include "../DVForwardDeclarations.h"
#include "../toolbox/KeyValueStore.h"
#include "../toolbox/TimeHelper.h"

namespace dv {

	class SimJob {
	public:
		static constexpr int kShellBufferSize = 1024;

		SimJob(DV *dvl_ptr, dv::id_type appid, dv::id_type target_nr,
			   std::unique_ptr<toolbox::KeyValueStore> parameters);
        
        SimJob(DV *dvl_ptr, dv::id_type jobid);

		std::string getParameter(const std::string &key) const;
		void setParameter(const std::string &key, const std::string &value);

		const std::unordered_map<std::string, std::string> &getParameters() const;
		void extendParameters(const std::unordered_map<std::string, std::string> &additional_parameters);

		void printParameters(std::ostream *out) const;

		bool isValidJob() const;
		void invalidateJob();

		dv::id_type getJobId() const;
		void setJobId(dv::id_type jobid);

		dv::id_type getGniRank() const;
		void setGniRank(dv::id_type gnirank);

		/**
		 * appid ==client id
		 */
		dv::id_type getAppId();


        /**
         * Return the last nr we have seen from this simulation. If no nrs have
           been seen till the function call, then the sim_start_nr_ is returned.
         */
        dv::id_type getCurrentNr() const;


		/**
		 * note: start is in normalized id number space
		 */
		dv::id_type getSimStart() const;
		void setSimStart(dv::id_type start);


		/**
		 * note: stop is in normalized id number space
		 */
		dv::id_type getSimStop() const;
		void setSimStop(dv::id_type stop);


		void handleSimulatorFileCreate(const std::string &name, FileDescriptor *cache_entry);
		void handleSimulatorFileClose(const std::string &name, FileDescriptor *cache_entry);

		void handleClientFileOpen(dv::id_type nr);

        bool hasToBeKilled();

        double getLastTau(); 

		void terminate();

		/**
		 * in seconds
		 */
		double getSetupDuration() const;

		const std::vector<double> &getTaus() const;

		bool isPrefetched() const;
		void setPrefetched(bool prefetched);

		void prepare();

		/**
		 * willProduceFile and willProduceNr return true only for files that will
		 * be produced in the future by the SimJob. False is returned for already
		 * produced files/time steps.
		 */
		bool willProduceFile(const std::string &filename) const;
		bool willProduceNr(dv::id_type nr) const;

		/**
		 * These functions return true if a file/timestep will be produced or
		 * already has been produced by the simulation.
		 */
		bool fileIsInSimulationRange(const std::string &filename) const;
		bool nrIsInSimulationRange(dv::id_type nr) const;

		dv::id_type launch();

		const std::string &getRedirectPath_result() const;
		const std::string &getRedirectPath_checkpoint() const;
    
        bool isPassive();

	protected:
		DV *dv_ptr_;
		dv::id_type appid_;

		dv::id_type jobid_ = 0;
		dv::id_type sysjobid_ = 0;
		std::string jobname_;

		bool valid_job_ = true;

		dv::id_type gnirank_ = 0;

		dv::id_type sim_start_nr_ = 0;
		dv::id_type sim_stop_nr_ = 0;

		std::unique_ptr<toolbox::KeyValueStore> parameters_;

		toolbox::TimeHelper::time_point_type start_time_ = toolbox::TimeHelper::now();
		toolbox::TimeHelper::time_point_type last_time_ = start_time_;
		double setup_duration_;

        dv::id_type last_nr_ = -1;

        //Killing simulations
        dv::id_type time_to_kill_;
        dv::id_type min_sim_target_;
        //dv::id_type last_accessed_restart_interval_=0;
        //dv::id_type kill_after_this_restart_interval_=0;

		dv::counter_type files_ = 0;

		// normalized ids
		std::unordered_set<dv::id_type > produced_file_numbers_;

		bool is_prefetched_ = false;
        bool is_passive_ = false;

		std::vector<double> taus_;

		std::string redirect_path_result_;
		std::string redirect_path_checkpoint_;

		bool createRedirectFolders();
		void removeRedirectFolders();
	};

}

#endif //DV_JOBS_SIMJOB_H
