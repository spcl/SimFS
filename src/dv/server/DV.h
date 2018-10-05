//
// Created by Pirmin Schmid on 13.04.17.
//

#ifndef DV_SERVER_DV_H_
#define DV_SERVER_DV_H_


#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

#include "../DVBasicTypes.h"
#include "../DVForwardDeclarations.h"
#include "DVConfig.h"
#include "DVStats.h"
#include "ClientDescriptor.h"
#include "JobQueue.h"
#include "../caches/filecaches/FileCache.h"
#include "../simulator/Simulator.h"
#include "../simulator/SimJob.h"
#include "../DVLog.h"
#include "../toolbox/StatisticsHelper.h"


#define ERROR 0
#define INFO 1
#define SIMULATOR 2
#define CLIENT 3
#define CACHE 4
#define WARNING 5
#define PREFETCHER 6


namespace dv {

	class DV {
	public:
		static constexpr int kListenBacklog = 4; // used for each listening socket, may be adjusted
		static constexpr int kMaxBufferLen = 4096;

        toolbox::TimeHelper::time_point_type start_time_;

		DV(std::unique_ptr<DVConfig> config);

		DVConfig *getConfigPtr() const;

		DVStats *getStatsPtr(); // must not be const

		/**
		 * DV takes ownership of the simulator and the cache objects.
		 * This assures that they are around as long as DVL is around.
		 */

		Simulator *getSimulatorPtr() const;
		void setSimulatorPtr(std::unique_ptr<Simulator> simulator_ptr);

		FileCache *getFileCachePtr() const;
		void setFileCachePtr(std::unique_ptr<FileCache> cache_ptr);

		const std::string getIpAddress() const;

		const std::string &getSimPort() const;

		const std::string &getClientPort() const;

		const std::string &getDvlJobid() const;

		dv::id_type getNewRank();

		const std::string &getRedirectPath() const;

        void setPassive();
        bool isPassive();
    
        void setFileIndex(toolbox::KeyValueStore * idx);
        void indexFile(std::string filename);

		/**
		 * indexJob, enqueueJob(), and registerClient() take ownership of the provided objects
		 * (sink; see unique_ptr) and keep the objects around as long as needed.
         * indexJob only register the job, while enqueueJob indexes and launches
         * it (i.e., enqueue in JobQueue).
		 *
		 * on the other hand, the lookup functions findSimJob(), findSimulationProducingFile()
		 * and findClientDescriptor() return raw pointers without ownership for the user.
		 * Note: they also may be == nullptr to indicate that the job or client was
		 * not found.
		 */

		void indexJob(dv::id_type id, std::unique_ptr<SimJob> job);
		void enqueueJob(dv::id_type id, std::unique_ptr<SimJob> job);
		bool isSimJobRunning(dv::id_type id) const;
		SimJob *findSimJob(dv::id_type job_id); // not const since client may adjust simjob

		/**
		 * These 2 functions report only true if the simulation will produce the file / timestep nr
		 * in the future. This is a suitable check if a file is requested by a client.
		 * There, we want to assure that the file has not yet already been deleted after being produced.
		 *
		 * Note: both, currently running and queued jobs, are checked.
		 */
		SimJob *findSimulationProducingFile(const std::string &filename); // not const since client may adjust simjob
		SimJob *findSimulationProducingNr(dv::id_type nr); // not const since client may adjust simjob

		/**
		 * These 2 functions report true if the simulation will produce, or has already produced
		 * the file. This is a suitable check during prefetch assessment. Otherwise, DV may start lots
		 * of unnecessary prefetching simulations, in particular if the client access trajectory is
		 * backwards.
		 *
		 * Note: both, currently running and queued jobs, are checked.
		 */
		SimJob *findSimulationWithFileInRange(const std::string &filename); // not const since client may adjust simjob
		SimJob *findSimulationWithNrInRange(dv::id_type nr); // not const since client may adjust simjob

		/**
		 * used to evaluate vertical prefetching limit
		 */
		dv::counter_type getNumberOfPrefetchingJobs(dv::id_type client);

        /* deindexJob just removes the job from the index.
         * removeJob removes it from the index and frees space in the JobQuee
         * (it can lead to the launch of a new queued simulation).
         */

        void deindexJob(dv::id_type id);
		void removeJob(dv::id_type id);

		void registerClient(dv::id_type appid, std::unique_ptr<ClientDescriptor> client);
		void unregisterClient(dv::id_type appid);
		ClientDescriptor *findClientDescriptor(dv::id_type appid); // not const since client may adjust the client descriptor

		void run();
		
		void quit();

		const toolbox::KeyValueStore &getStatusSummary();


		// this is currently mainly for testing purpose of set_info and get_info
		// no interaction with actual DV state yet.
		void extendedApiSetInfo(std::string key, dv::id_type value);
		dv::id_type extendedApiGetInfo(std::string key, dv::id_type default_value);

	private:
		// associated key elements
		std::unique_ptr<DVConfig> config_;
		std::unique_ptr<Simulator> simulator_ptr_;
		std::unique_ptr<FileCache> filecache_ptr_;
    
        /* if true, the server accepts all the incoming simulation requests */
        bool passive_mode_ = false;
        
		toolbox::KeyValueStore statusSummary_;
        toolbox::KeyValueStore * file_idx_ = NULL;
        

		std::string ip_address_;

		std::string redirect_path_;

		// note: no direct access to jobqueue is given to clients of DV to avoid any memory leaks/dangling pointers,
		// and job duplicates, the job queue methods are wrapped by this DV class, in particular enqueue.
		// enqueueJob:
		// 1) store job data structure (unique_ptr) in simulations_ map
		// 2) submit a non-owning raw pointer to this data structure to the actual queue,
		//    which will only store a pointer if needed
		// finished:
		// 1) let queue handle its things
		// 2) free data structure (unique_ptr) in simulations, which leads to memory release.
		JobQueue jobqueue_;

		std::unordered_map<dv::id_type, std::unique_ptr<SimJob>> simulation_jobs_;

		// messageHandlers

		int sim_socket_ = 0;
		int client_socket_ = 0;

		std::unordered_map<dv::id_type, std::unique_ptr<ClientDescriptor>> clients_;

		// this is currently mainly for testing purpose of set_info and get_info
		// no interaction with actual DV state yet.
		std::unordered_map<std::string, dv::id_type> extended_api_info_map_;

		// debug

		DVStats stats_;

		dv::id_type conncount_ = 0;

		dv::id_type message_count_ = 0;

		// for StopServerMessageHandler
		volatile bool quit_requested_ = false;

		bool createRedirectFolder();
		void removeRedirectFolder();

		int startServerPart(const std::string &port);

		void startServer();

		void stopServer();

		void printStats();

		void printAccessTrace();

		void finish();
	};

}

#endif //DV_SERVER_DV_H
