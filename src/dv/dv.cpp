//
// Handles input arguments and environment variables
// and starts the server
//
// 2017-04-12 / 2017-09-25 Pirmin Schmid
//

#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <sys/types.h>
#include <unistd.h>


#include "dv.h"

#include "getopt/dv_cmdline.h"

#include "toolbox/Version.h"
#include "toolbox/Logger.h"

#include "caches/filecaches/FileCacheUnlimited.h"
#include "caches/filecaches/FileCacheLRU.h"
#include "caches/filecaches/FileCacheARC.h"
#include "caches/filecaches/FileCacheLIRS.h"
#include "caches/filecaches/FileCacheBCL.h"
#include "caches/filecaches/FileCacheDCL.h"
#include "caches/filecaches/FileCachePLRU.h"
#include "caches/filecaches/FileCachePBCL.h"
#include "caches/filecaches/FileCachePDCL.h"
#include "caches/filecaches/FileCacheFifoWrapper.h"


using namespace std;
using namespace dv;
using namespace toolbox;


namespace {
/* global namespace */
volatile std::sig_atomic_t globalSignalStatus;

bool stop_requested_predicate() {
    return globalSignalStatus != 0;
}

void signalHandler(int signum) {
    globalSignalStatus = signum;
}
}


DV * DVCreate(gengetopt_args_info &args_info, const string &config_file) {

    unique_ptr<DVConfig> dv_config = make_unique<DVConfig>();

    //--- DV config: direct settings -------------------------------------------
    dv_config->dv_debug_output_on_ = false;
    dv_config->filecache_debug_output_on_ = false;
    dv_config->filecache_details_debug_output_on_ = false;

    dv_config->stop_requested_predicate_ = stop_requested_predicate;


    //--- DV config: config file ------------------------------------------------

    if (!dv_config->loadConfigFile(config_file)) {
        LOG(ERROR, 0, "Could not read the config file " + config_file + ".");
        return NULL;
    }

    if (!dv_config->init()) {
        LOG(ERROR, 0, "Config file init(): did not pass.");
        return NULL;
    }

    /* Installing signal handlers */
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);


    //--- additional DV configuration: command line arguments -------------------
    // note: they overwrite any config given in the config file
    // and in environment variables

    //--- server

    if(args_info.hostname_given) {
        dv_config->dv_hostname_ = args_info.hostname_arg;
    }

    if (args_info.simulator_port_given) {
        dv_config->dv_sim_port_ = args_info.simulator_port_arg;
    }

    if (args_info.client_port_given) {
        dv_config->dv_client_port_ = args_info.client_port_arg;
    }


    //--- paths

    if (args_info.simulator_given) {
        dv_config->sim_config_path_ = args_info.simulator_arg;
    }

    if (args_info.checkpoints_given) {
        dv_config->sim_checkpoint_path_ = args_info.checkpoints_arg;
    }

    if (args_info.results_given) {
        dv_config->sim_result_path_ = args_info.results_arg;
    }

    if (args_info.dummy_given) {
        dv_config->sim_temporary_redirect_path_ = args_info.dummy_arg;
    }

    //--- filecache

    if (args_info.filecache_type_given) {
        dv_config->filecache_type_ = args_info.filecache_type_arg;
    }

    if (args_info.filecache_size_given) {
        dv_config->filecache_size_ = args_info.filecache_size_arg;
    }

    if (args_info.filecache_fifo_size_given) {
        dv_config->filecache_fifo_queue_size_ = args_info.filecache_fifo_size_arg;
    }

    if (args_info.filecache_lir_set_size_given) {
        dv_config->filecache_lir_set_size_ = args_info.filecache_lir_set_size_arg;
    }

    if (args_info.filecache_mrus_given) {
        dv_config->filecache_protected_mrus_ = args_info.filecache_mrus_arg;
    }

    if (args_info.filecache_penalty_given) {
        dv_config->filecache_penalty_factor_ = args_info.filecache_penalty_arg;
    }


    //--- prefetching

    if (args_info.prefetch_intervals_given) {
        // this sets vertical to the value and horizontal to 1
        // following the initial settings (compatibility)
        dv_config->dv_max_vertical_prefetching_intervals_ = args_info.prefetch_intervals_arg;
        dv_config->dv_max_horizontal_prefetching_intervals_ = 1;
    }

    if (args_info.vertical_prefetch_interval_given) {
        dv_config->dv_max_vertical_prefetching_intervals_ = args_info.vertical_prefetch_interval_arg;
    }

    if (args_info.horizontal_prefetch_interval_given) {
        dv_config->dv_max_horizontal_prefetching_intervals_ = args_info.horizontal_prefetch_interval_arg;
    }

    if (args_info.parallel_simulators_given) {
        dv_config->dv_max_parallel_simjobs_ = args_info.parallel_simulators_arg;
    }

    if (args_info.sim_kill_threshold_given) {
        dv_config->sim_kill_threshold_ = args_info.sim_kill_threshold_arg;
    }

    //--- final config check & create DV ---------------------------------------

    if(!dv_config->assure_config_ok()) {
        LOG(ERROR, 0, "Configuration is not complete. Cannot create DV.");
        return NULL;
    }

    dv_config->print(&std::cout);

    DV * dv = new DV(move(dv_config));


    //--- simulator configuration ----------------------------------------------

    dv->setSimulatorPtr(make_unique<Simulator>(dv));
    dv->getSimulatorPtr()->scanRestartFiles();

    //--- file cache configuration ---------------------------------------------

    FileCache::FileCacheType filecache_type = FileCache::getFileCacheType(dv->getConfigPtr()->filecache_type_);
    FileCacheLRU::ID_type embedded_cache_size = dv->getConfigPtr()->filecache_embedded_cache_size_;

    FileCacheLRU::ID_type cache_protected_mrus = dv->getConfigPtr()->filecache_protected_mrus_;
    double penalty_factor = dv->getConfigPtr()->filecache_penalty_factor_;

    std::unique_ptr<FileCache> embedded_cache;

    switch (filecache_type) {
    case FileCache::kUnlimited:
        embedded_cache = std::make_unique<FileCacheUnlimited>(dv);
        break;
    case FileCache::kLRU:
        embedded_cache = std::make_unique<FileCacheLRU>(dv, embedded_cache_size);
        break;
    case FileCache::kARC:
        embedded_cache = std::make_unique<FileCacheARC>(dv, embedded_cache_size);
        break;
    case FileCache::kLIRS:
        embedded_cache = std::make_unique<FileCacheLIRS>(dv, embedded_cache_size, dv->getConfigPtr()->filecache_lir_set_size_);
        break;
    case FileCache::kBCL:
        embedded_cache = std::make_unique<FileCacheBCL>(dv, embedded_cache_size, cache_protected_mrus);
        break;
    case FileCache::kDCL:
        embedded_cache = std::make_unique<FileCacheDCL>(dv, embedded_cache_size, cache_protected_mrus);
        break;
    case FileCache::kACL:
        LOG(ERROR, 0, "Cache type ACL not yet implemented");
        return NULL;
        break;
    case FileCache::kPLRU:
        embedded_cache = std::make_unique<FileCachePLRU>(dv, embedded_cache_size);
    case FileCache::kPBCL:
        embedded_cache = std::make_unique<FileCachePBCL>(dv, embedded_cache_size, cache_protected_mrus, penalty_factor);
        break;
    case FileCache::kPDCL:
        embedded_cache = std::make_unique<FileCachePDCL>(dv, embedded_cache_size, cache_protected_mrus, penalty_factor);
        break;
    case FileCache::kPACL:
        LOG(ERROR, 0, "Cache type PACL not yet implemented");
        return NULL;
        break;
    default:
        LOG(ERROR, 0, "Cache type " + dv->getConfigPtr()->filecache_type_ + " unknown.");
        return NULL;

    }

    if (0 < dv->getConfigPtr()->filecache_fifo_queue_size_) {
        dv->setFileCachePtr(std::make_unique<FileCacheFifoWrapper>(dv, std::move(embedded_cache),
                            dv->getConfigPtr()->filecache_fifo_queue_size_));
    } else {
        dv->setFileCachePtr(std::move(embedded_cache));
    }

    dv->getFileCachePtr()->initializeWithFiles();

    return dv;

}

