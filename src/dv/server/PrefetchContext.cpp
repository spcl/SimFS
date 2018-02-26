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


void PrefetchContext::handleMiss(const std::string &filename){
    parsims_ = 1;
    check_for_prefetch(filename);
}


void PrefetchContext::handleHit(const std::string &filename){
    check_for_prefetch(filename);
}

void PrefetchContext::check_for_prefetch(const std::string &filename) {

    dv::id_type nr = dv_->getSimulatorPtr()->result2nr(filename);


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

            client_->newSimulation(last_nr_, last_nr_ + simlen, "");
            last_nr_ = dv_->getSimulatorPtr()->getNextCheckpointNr(last_nr_ + simlen);
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

