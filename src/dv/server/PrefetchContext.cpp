//
// 01/2018: SDG
//


#include <cmath>

#include "PrefetchContext.h"
#include "DV.h"
#include "ClientDescriptor.h"
#include "../DVLog.h"

#define MAX(a, b) ((a>b) ? a : b)
#define MIN(a, b) ((a<b) ? a : b)

namespace dv {

dv::id_type PrefetchContext::getStride(){
    if (state_!=STEADY) return 0;
    return stride_;
}

PrefetchContext::PrefetchContext(ClientDescriptor * client, dv::id_type appid) :
    client_(client), appid_(appid) {
    dv_ = client->dv_;
}


void PrefetchContext::handleMiss(dv::id_type target_nr, std::string parameters){
    parsims_ = 1;


    /* extend the simulation interval according to the prefetcher:
     *  MISSING - EXTEND_LEFT -> MISSING + EXTEND_RIGHT
     *  At most one of the two extend_* is !=0:
     *   FW trajectory: extend_left; BW trajectory: extend_right;
     *  This is needed because if stride > restart interval then 
     *  the prefetcher never gets a chance to activate since it does
     *  not have taus.
     */
    dv::id_type stride = getStride();
    dv::id_type extend_left = stride<0 ? -stride : 0;
    dv::id_type extend_right = stride>0 ? stride : 0;

    std::unique_ptr<SimJob> simjob = client_->newSimulation(target_nr - extend_left, target_nr + extend_right, parameters);

    if (simjob==nullptr){
        LOG(ERROR, 0, "Unable to create a simulation to serve this miss!");
        return;
    }

    

    if (stride>=0) {
        last_nr_ = simjob->getSimStop();
        std::cout << "Target nr: " << target_nr << "; extend_right: " << extend_right << "; last_nr: " << last_nr_ << std::endl;
    }else{
        last_nr_ = simjob->getSimStart();
    }

    dv::id_type simjobid = simjob->getJobId();
    dv_->enqueueJob(simjobid, std::move(simjob));
    

    /* we keep checking because the prefetchcontext could be still valid. 
     * E.g., the restart interval I is smaller than the stride S. 
     * Now assume that we are at the first simulation (prefetcher did not fire
     * any simulation yet). The next access will be again a miss because
     * the first simulation ended before, but still the prefetch context
     * could be valid (i.e., same stride). */
    check_for_prefetch(target_nr);
}


void PrefetchContext::handleHit(dv::id_type target_nr, std::string parameters){
    check_for_prefetch(target_nr);
}

void PrefetchContext::check_for_prefetch(dv::id_type nr) {

    dv::id_type newstride = nr - last_access_;
    last_access_ = nr;


    LOG(PREFETCHER, 0, "State: " + std::to_string(state_) + \
        " (current stride: " + std::to_string(stride_) + \
        " new stride: " + std::to_string(newstride) + ")");



    if (state_==DISABLED) {
        state_ = TRANSIENT;
        return;

    } else if (state_ == TRANSIENT) {
        stride_ = newstride;
        state_ = STEADY;

    } else { /* state_ is STEADY */

        if (stride_ != newstride) {
            reset();
            LOG(PREFETCHER, 0, "Invalidating! New stride: " + std::to_string(newstride) + \
                "; expected: " + std::to_string(stride_));
            return;
        }

        if (stride_>0) forward_prefetch(nr);
        else backward_prefetch(nr);
    }
}

void PrefetchContext::reset() {
    stride_ = 0;
    state_ = DISABLED;
    last_nr_ = -1;
    last_access_ = -1;
    parsims_ = 1;
}

void PrefetchContext::forward_prefetch(dv::id_type nr) {
    double simalpha = client_->sim_profiler_.getAlpha();
    double simtau = client_->sim_profiler_.getTau();
    double mytau = client_->cli_profiler_.getTau();


    if (last_nr_ == -1 || nr > last_nr_) {
        /* if we don't have a last_nr, then the simulation was not prefetched,
           and it will be running until the next restart; */
        last_nr_ = dv_->getSimulatorPtr()->getNextCheckpointNr(nr);
    }

    dv::id_type critical_step = \
        (last_nr_ / stride_ - (simalpha / MAX(simtau/parsims_, mytau)))*stride_;


    LOG(PREFETCHER, 0, "sim.alpha: " + std::to_string(simalpha) + \
        "; sim.tau: " + std::to_string(simtau) + \
        "; client.tau: " + std::to_string(mytau));
    LOG(PREFETCHER, 0, "critical_step: " + std::to_string(critical_step) + \
        "; last_nr: " + std::to_string(last_nr_) + \
        "; nr: " + std::to_string(nr) + \
        "; stride: " + std::to_string(stride_));


    if (nr > critical_step) {
        /* PREFETCH!!! */

        int simlen = ceil(simalpha / MAX(simtau, mytau))*stride_;
        for (int i=0; i<parsims_; i++) {
            LOG(PREFETCHER, 0, "New simulation! " + \
                std::to_string(last_nr_) + " -> " + std::to_string(last_nr_ + simlen) + "; " + \
                "simlen: " + std::to_string(simlen) + "; " + \
                "nextcheckpoint: " + \
                std::to_string(dv_->getSimulatorPtr()->getNextCheckpointNr(last_nr_ + simlen)));

            dv::id_type start_from = dv_->getSimulatorPtr()->getNextCheckpointNr(last_nr_);
            std::unique_ptr<SimJob> simjob = client_->newSimulation(start_from, last_nr_ + simlen, "");
            if (simjob==NULL){
                LOG(WARNING, 0, "Cannot create prefetch simulation!");
            }else{
                last_nr_ = simjob->getSimStop();
                dv::id_type simjobid = simjob->getJobId();
                dv_->enqueueJob(simjobid, std::move(simjob));
            }
        }

        dv::id_type max_parsims = dv_->getConfigPtr()->dv_max_vertical_prefetching_intervals_;
        dv::id_type optimal = (dv::id_type) MIN(ceil(simtau / mytau), max_parsims);
        if (parsims_ < optimal) parsims_ = MIN(parsims_*2, optimal);
        else parsims_ = optimal;
        printf("PrefetchContext (FW): parsims: %li; optimal: %li; max: %li\n", parsims_, (dv::id_type) ceil(simtau / mytau), max_parsims);
    }

}

void PrefetchContext::backward_prefetch(dv::id_type nr) {
    printf("Backward prefetch not implented!\n");
}

}

