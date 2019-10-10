// 
// 01/2018: Prefetching (SDG)
// 09/2017: Adding support for range queries (only stubs) (PS)
// 04/2017: Porting/rewriting from SDG's python version (PS)
//

#include "ClientDescriptor.h"

#include <algorithm>
#include <cassert>
#include <iostream>

#include "DV.h"
#include "../caches/filecaches/FileDescriptor.h"
#include "../simulator/Simulator.h"
#include "../simulator/SimJob.h"

#include "../toolbox/KeyValueStore.h"
#include "../toolbox/StringHelper.h"

namespace dv {

ClientDescriptor::ClientDescriptor(DV *dv, dv::id_type appid) :
    dv_(dv), appid_(appid), prefetcher_(this, appid),
    max_prefetching_intervals_(dv->getConfigPtr()->dv_max_prefetching_intervals_)
{;}

void ClientDescriptor::profile(FileDescriptor * cache_entry) {
    if (notified_ || cache_entry != nullptr) {
        double newtau = cli_profiler_.newTau();
        LOG(PREFETCHER, 0, "adding tau: " + std::to_string(newtau) + "; current tau: " + std::to_string(cli_profiler_.getTau()));
        notified_ = false;
    }
}

bool ClientDescriptor::handleOpen(const std::string &filename,
                                  const std::vector<std::string> &parameters) {

    // note: parameters is not actually used at the moment


    /* we need the full get from the cache to trigger all actions in case
       of cache miss. Note: use put() with a new descriptor in case a
       nullptr was returned and use refresh, if a descriptor was found */
    FileDescriptor * cache_entry = dv_->getFileCachePtr()->get(filename);

    profile(cache_entry);

    /* if the cache entry is marked as prefetched now mark it as not */
    if (cache_entry != nullptr && cache_entry->isFilePrefetched()) {
        cache_entry->setFilePrefetched(false);
    }

    SimJob *already_simulating_job = dv_->findSimulationProducingFile(filename);
    bool is_being_simulated = already_simulating_job != nullptr;
    bool is_miss = cache_entry == nullptr && !is_being_simulated;

    if (cache_entry == nullptr) {
        /*NOTE: even if a file is being simulated by another simjob, that simjob
          could still need time to create this file, so the cache_entry could
          be null. */

        //create the file descriptor and put it in cache as not available
        std::string fullpath = toolbox::StringHelper::joinPath(dv_->getConfigPtr()->sim_result_path_, filename);
        std::unique_ptr<FileDescriptor> descriptor = std::make_unique<FileDescriptor>(filename, fullpath);
        descriptor->setFileAvailable(false);
        descriptor->lock();
        dv_->getFileCachePtr()->put(filename, std::move(descriptor));
    } else cache_entry->lock();

    dv::id_type target_nr = dv_->getSimulatorPtr()->result2nr(filename);
    LOG(CLIENT, 0, "Client " + std::to_string(appid_) + " is opening " + filename + "; nr: " + std::to_string(target_nr));

    toolbox::TimeHelper::time_point_type now = toolbox::TimeHelper::now();
    double time = toolbox::TimeHelper::milliseconds(dv_->start_time_, now);


    if (is_miss) { 

        LOG(CLIENT, 0, "MISS: Restarting simulation! Params: " + parameters[0]);
        LOG(CLIENT, 0, "[EVENT][" + std::to_string(appid_) + "] CLIENT_OPEN " + filename + " MISS: " +  std::to_string(time));


        /* prefetcher is in charge to restart the simulation */
        prefetcher_.handleMiss(target_nr, parameters[0]); 

        //logs & stats
        dv_->getStatsPtr()->incMisses();
    
        return false;
    } else { 

        prefetcher_.handleHit(target_nr, parameters[0]);
 
        if (cache_entry!=NULL && !cache_entry->isFileUsedBySimulator()){
            // is a full hit: the data is available /
            LOG(CLIENT, 0, "HIT! Data is available!");
            LOG(CLIENT, 0, "[EVENT][" + std::to_string(appid_) + "] CLIENT_OPEN " + filename + " HIT: " +  std::to_string(time));

            return true;
        }else if (is_being_simulated){
            dv_->getStatsPtr()->incWaiting();
            LOG(CLIENT, 0, "HIT (WAIT): Data already being simulated by: " + std::to_string(already_simulating_job->getJobId()));
            LOG(CLIENT, 0, "[EVENT][" + std::to_string(appid_) + "] CLIENT_OPEN " + filename + " HIT_WAIT: " +  std::to_string(time));

            already_simulating_job->handleClientFileOpen(target_nr);
            return false;
        }else{
            assert(cache_entry != NULL);
            // the simulator is currently writing this file! 
            LOG(CLIENT, 0, "HIT (WAIT): Simulator is writing on this file!");
            LOG(CLIENT, 0, "[EVENT][" + std::to_string(appid_) + "] CLIENT_OPEN " + filename + " HIT_WAIT_W: " +  std::to_string(time));

            //FIXME: not sure if this is still reachable and if it will succeed (e.g., who notifies the client?)
            return false;
        }

        /*
        This does not work well because the hits following a miss will be seen
        as "HIT (WAIT)", instead of "HIT!", if the resimulation is going to 
        produce these files. 
        if (is_being_simulated) {
            // there is a simulation that will produce this file 
            //dv::id_type waiting_time = requested_nr - already_simulating_job->getCurrentNr();
            dv_->getStatsPtr()->incWaiting();
            LOG(CLIENT, 0, "HIT (WAIT): Data already being simulated by: " + std::to_string(already_simulating_job->getJobId()));
            already_simulating_job->handleClientFileOpen(target_nr);
            return false;

        } else if (cache_entry!=nullptr && cache_entry->isFileUsedBySimulator()) {
            // the simulator is currently writing this file! 
            LOG(CLIENT, 0, "HIT (WAIT): Simulator is writing on this file!");
            return false;

        } else {
            // is a full hit: the data is available /
            LOG(CLIENT, 0, "HIT! Data is available!");
            return true;
        }
        */
        
    }


    //std::cout << "# " << dv_->getConfigPtr()->dv_stat_label_
    //		  << " open " << openop_
    //		  << " " << cli_profiler_.rstring()
    //		  << " " << sim_profiler_.rstring() << std::endl;
    //openop_++;

}


std::unique_ptr<SimJob> ClientDescriptor::newSimulation(dv::id_type target_nr, dv::id_type simstop, std::string strparams) {
    //create job parameters
    std::unique_ptr<toolbox::KeyValueStore> jobparameters = std::make_unique<toolbox::KeyValueStore>();
    jobparameters->fromString(strparams);

    //create the job for simulating the missing file
    std::unique_ptr<SimJob> simjob = dv_->getSimulatorPtr()->generateSimJobForRange(appid_, target_nr, simstop, std::move(jobparameters));

    if (simjob->isValidJob()) {
        //dv_->getStatsPtr()->incResim(dv_->getSimulatorPtr()->getPrevRestartDiff(filename));
        dv::id_type simjobid = dv_->getNewRank();
        simjob->setJobId(simjobid);
        simjob->handleClientFileOpen(target_nr);
        //std::cout << "Cost: [" << appid_ << " 1 " << requested_nr - simjob->getSimStart() << "]" << std::endl;

        LOG(CLIENT, 0, "New simulation: ID: " + std::to_string(simjobid) + " range: " + std::to_string(simjob->getSimStart()) + " -> " + std::to_string(simjob->getSimStop()));

        //dv_->enqueueJob(simjobid, std::move(simjob));
        // do not access simjob after this point againA
        //LOG(CLIENT, 1, "Simulation added to queue with id " + std::to_string(simjobid));
        return simjob;
    } else {
        LOG(WARNING, 0, "Simulation non needed or not possible (invalid simjob)!");
        return nullptr;
    }
}

void ClientDescriptor::handleNotification(SimJob *simjob) {
    dv::id_type jobid = simjob->getJobId();
    auto it = known_sims_.find(jobid);
    if (it == known_sims_.end()) {
        // unknown
        //LOG(CLIENT, 1, "Simulation is *NOT* known, adding it to the known ones!");
        known_sims_.emplace(jobid);
        sim_profiler_.addAlpha(simjob->getSetupDuration());
    
        printf("SIMULATION NOT KNOW! Adding alpha: %lf\n", simjob->getSetupDuration());

        // this does not happen when prefetching is going, so it's
        // safe to get the taus from the simulator history.
        // Note: this is needed especially in the bw direction, where
        // you don't get notifications for the files produced before
        // the one you requested.
        //TODO: taus should be selected according to the stride!!
        //sim_profiler_.clearTaus();
        sim_profiler_.extendTaus(simjob->getTaus());

    } else {
        // known: register the tau
        //LOG(CLIENT, 1, "Simulation is known");
        sim_profiler_.newTau();
    }

    cli_profiler_.reset();
    notified_ = true;
    LOG(CLIENT, 0, "Client notified: Sim profile: " + sim_profiler_.toString());
}




/**
 * signals below bottom with -1
 * signals above ceiling with -2
 */
dv::id_type ClientDescriptor::next_target_nr(dv::id_type current, dv::id_type direction) {
    if (direction < 0) {
        if (current == 1) {
            // bottom was already reached
            return -1;
        }
        current -= 2;
        return dv_->getSimulatorPtr()->getCheckpointNr(current) + 1;
        // note: this will never produce target_nr < 0
    } else {
        dv::id_type  next = current + 2;
        next = dv_->getSimulatorPtr()->getNextCheckpointNr(next) - 1;
        if (next == (current - 1)) {
            // ceiling was already reached
            return -2;
        }
        return next;
    }
}


void ClientDescriptor::handleRangeRequest(dv::id_type flag, std::unique_ptr<ClientDescriptor::RangeRequest> request) {
    if ((flag & kRequestRangeFlagFirst) == kRequestRangeFlagFirst) {
        range_requests_.clear();
    }

    range_requests_.push_back(std::move(request));

    if ((flag & kRequestRangeFlagLast) == kRequestRangeFlagLast) {
        // TODO: actual action of handling requested ranges
    }
}

}
