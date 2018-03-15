//
// 04/2017: Porting/rewriting from SDG's python version (PS) 
//
// network handling follows this guide (code in Public Domain) with modifications:
// http://beej.us/guide/bgnet/output/html/multipage/index.html
//

#include "DV.h"

#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>

#include <cassert>
#include <cstring>
#include <vector>

#include "MessageHandler.h"
#include "MessageHandlerFactory.h"
#include "../caches/filecaches/FileCache.h"
#include "../simulator/Simulator.h"
#include "../toolbox/StatisticsHelper.h"
#include "../toolbox/StringHelper.h"
#include "../toolbox/FileSystemHelper.h"
#include "../toolbox/TimeHelper.h"
#include "../toolbox/NetworkHelper.h"


namespace dv {

DV::DV(std::unique_ptr<DVConfig> config) : config_(std::move(config)) {
    jobqueue_.setMaxSimJobs(config_->dv_max_parallel_simjobs_);
    ip_address_ = config_->dv_hostname_;
}

DVConfig *DV::getConfigPtr() const {
    return config_.get();
}

DVStats *DV::getStatsPtr() {
    return &stats_;
}

Simulator *DV::getSimulatorPtr() const {
    return simulator_ptr_.get();
}

void DV::setSimulatorPtr(std::unique_ptr<Simulator> simulator_ptr) {
    simulator_ptr_ = std::move(simulator_ptr);
}

FileCache *DV::getFileCachePtr() const {
    return filecache_ptr_.get();
}

void DV::setFileCachePtr(std::unique_ptr<FileCache> cache_ptr) {
    filecache_ptr_ = std::move(cache_ptr);
}

void DV::setPassive(){
    passive_mode_ = true;
}

bool DV::isPassive(){
    return passive_mode_;
}



void DV::run() {
    if (!createRedirectFolder()) {
        std::cerr << "ERROR while creating redirect folder. Check path: " << config_->sim_temporary_redirect_path_ << std::endl;
        exit(1);
    }

    startServer();

    fd_set read_fds;
    int max_fd = sim_socket_ > client_socket_ ? sim_socket_ : client_socket_;

    std::string delimiter(":");



    while (!config_->stop_requested_predicate_() && !quit_requested_) {
        FD_ZERO(&read_fds);
        FD_SET(sim_socket_, &read_fds);
        FD_SET(client_socket_, &read_fds);

        // TODO: add timeout to select
        int nr = select(max_fd + 1, &read_fds, nullptr, nullptr, nullptr);
        if (0 < nr) {
            char buf[kMaxBufferLen];
            size_t buflen = sizeof(buf);
            char *bufptr = &buf[0];
            memset(bufptr, 0, buflen);

            struct sockaddr addr;
            socklen_t addrlen = sizeof(addr);
            if (FD_ISSET(sim_socket_, &read_fds)) {
                int socket = accept(sim_socket_, &addr, &addrlen);
                if (socket != -1) {

                    ssize_t len = recv(socket, bufptr, buflen - 1, 0); // to assure having always a 0 at the end
                    // note: len == 0 just indicates a closed socket from client
                    if (0 < len) {

                        LOG(INFO, 1, "Server: got message (%s)!" + std::string(bufptr))

                        std::vector<std::string> params;
                        toolbox::StringHelper::splitCStr(&params, bufptr, delimiter);

                        ++message_count_;
                        MessageHandlerFactory::runMessageHandler2(this, socket, MessageHandlerFactory::kSimulator, params);
                    } else {
                        LOG(INFO, 1, "Socket closed by simulator.");
                    }
                } else {
                    LOG(ERROR, 0, "Server: accept error: " + std::to_string(errno));
                }
            }

            if (FD_ISSET(client_socket_, &read_fds)) {
                int socket = accept(client_socket_, &addr, &addrlen);
                if (socket != -1) {

                    ssize_t len = recv(socket, bufptr, buflen - 1, 0); // to assure having always a 0 at the end
                    // note: len == 0 just indicates a closed socket from client
                    if (0 < len) {
                        if (config_->dv_debug_output_on_) {
                            std::cout << "Server: got message on client server socket " << client_socket_ << ": " << bufptr << std::endl;
                        }

                        std::vector<std::string> params;
                        toolbox::StringHelper::splitCStr(&params, bufptr, delimiter);

                        //MessageHandler *h = MessageHandlerFactory::createMessageHandler(this, socket, MessageHandlerFactory::kClient, params);
                        //h->serve();
                        //delete h;
                        ++message_count_;
                        MessageHandlerFactory::runMessageHandler2(this, socket, MessageHandlerFactory::kClient, params);
                    } else {
                        if (config_->dv_debug_output_on_) {
                            std::cout << "Server: got message on client socket of length " << len
                                      << ": server socket closed by client" << std::endl;
                        }
                    }
                } else {
                    std::cerr << "Server: accept: client socket: error " << errno << std::endl;
                }
            }
        } else if (nr < 0) {
            // error
            if (!config_->stop_requested_predicate_() && !quit_requested_) {
                std::cerr << "Server: select: error " << nr << std::endl;
            }
        } else {
            // timeout
            // just let it block again after checking the stop_requested predicate
        }
    }

    std::cout << std::endl << "stop requested" << std::endl;
    finish();
}

void DV::quit() {
    quit_requested_ = true;
}

const toolbox::KeyValueStore &DV::getStatusSummary() {
    // update KV store
    statusSummary_.setInt("dv_message_count", message_count_);
    statusSummary_.setInt("dv_connection_count", conncount_);
    statusSummary_.extendMap(simulator_ptr_->getStatusSummary().getStoreMap());
    statusSummary_.extendMap(filecache_ptr_->getStatusSummary().getStoreMap());

    // note: assure that the store does not get too long.
    // It must be able to fit into one network message!

    return statusSummary_;
}

const std::string DV::getIpAddress() const {
    std::string ip = ip_address_;
    if (ip=="0.0.0.0") {
        std::vector<std::string> ips;
        toolbox::NetworkHelper::getAllIPs(ips);
        ip = "";
        for (auto lip : ips) {
            ip = ip + lip + ",";
        }
    }

    return ip;
}

const std::string &DV::getSimPort() const {
    return config_->dv_sim_port_;
}

const std::string &DV::getClientPort() const {
    return config_->dv_client_port_;
}

const std::string &DV::getDvlJobid() const {
    return config_->dv_batch_job_id_;
}

dv::id_type DV::getNewRank() {
    // rank ids are a 1 based sequence
    return ++conncount_;
}

const std::string &DV::getRedirectPath() const {
    return redirect_path_;
}

/* only index (used by the passive mode where the job is already running) */
void DV::indexJob(dv::id_type id, std::unique_ptr<SimJob> job) {
    simulation_jobs_[id] = std::move(job);
}

/* index & launch */
void DV::enqueueJob(dv::id_type id, std::unique_ptr<SimJob> job) {
    simulation_jobs_[id] = std::move(job);
    jobqueue_.enqueue(simulation_jobs_[id].get());
}

bool DV::isSimJobRunning(dv::id_type id) const {
    auto it = simulation_jobs_.find(id);
    return it != simulation_jobs_.end();
}

SimJob *DV::findSimJob(dv::id_type job_id) {
    auto it = simulation_jobs_.find(job_id);
    if (it == simulation_jobs_.end()) {
        return nullptr;
    }

    return it->second.get();
}

SimJob *DV::findSimulationProducingFile(const std::string &filename) {
    for (const auto &job : simulation_jobs_) {
        if (job.second->willProduceFile(filename)) {
            return job.second.get();
        }
    }

    for (SimJob *job : jobqueue_.queue()) {
        if (job->willProduceFile(filename)) {
            return job;
        }
    }

    return nullptr;
}

SimJob *DV::findSimulationProducingNr(dv::id_type nr) {
    for (const auto &job : simulation_jobs_) {
        if (job.second->willProduceNr(nr)) {
            return job.second.get();
        }
    }

    for (SimJob *job : jobqueue_.queue()) {
        if (job->willProduceNr(nr)) {
            return job;
        }
    }

    return nullptr;
}

SimJob *DV::findSimulationWithFileInRange(const std::string &filename) {
    for (const auto &job : simulation_jobs_) {
        if (job.second->fileIsInSimulationRange(filename)) {
            return job.second.get();
        }
    }

    for (SimJob *job : jobqueue_.queue()) {
        if (job->fileIsInSimulationRange(filename)) {
            return job;
        }
    }

    return nullptr;
}

SimJob *DV::findSimulationWithNrInRange(dv::id_type nr) {
    for (const auto &job : simulation_jobs_) {
        if (job.second->nrIsInSimulationRange(nr)) {
            return job.second.get();
        }
    }

    for (SimJob *job : jobqueue_.queue()) {
        if (job->nrIsInSimulationRange(nr)) {
            return job;
        }
    }

    return nullptr;
}

dv::counter_type DV::getNumberOfPrefetchingJobs(dv::id_type client) {
    dv::counter_type count = 0;
    for (const auto &job : simulation_jobs_) {
        if (job.second->isPrefetched() && job.second->getAppId() == client) {
            count++;
        }
    }

    for (SimJob *job : jobqueue_.queue()) {
        if (job->isPrefetched() && job->getAppId() == client) {
            count++;
        }
    }
    return count;
}

void DV::deindexJob(dv::id_type id) {
    simulation_jobs_.erase(id);
}

void DV::removeJob(dv::id_type id) {
    jobqueue_.handleJobTermination();
    simulation_jobs_.erase(id);
    // note: removal of the unique_ptr<> will then also free back the heap space of the simjob
}

void DV::registerClient(dv::id_type appid, std::unique_ptr<ClientDescriptor> client) {
    clients_[appid] = std::move(client);
}

void DV::unregisterClient(dv::id_type appid) {
    clients_.erase(appid);
    // note: removal of the unique_ptr<> will then also free back the heap space of the client
}

ClientDescriptor *DV::findClientDescriptor(dv::id_type appid) {
    auto it = clients_.find(appid);
    if (it == clients_.end()) {
        return nullptr;
    }

    return it->second.get();
}

void DV::extendedApiSetInfo(std::string key, dv::id_type value) {
    extended_api_info_map_[key] = value;
}

dv::id_type DV::extendedApiGetInfo(std::string key, dv::id_type default_value) {
    auto v = extended_api_info_map_.find(key);
    if (v != extended_api_info_map_.end()) {
        return v->second;
    }
    return default_value;
}


//--- private --------------------------------------------------------------

bool DV::createRedirectFolder() {
    if (config_->sim_temporary_redirect_path_.empty()) {
        std::cerr << "sim_temporary_redirect_path must not be empty" << std::endl;
        return false;
    }

    auto now_ns = std::chrono::nanoseconds(toolbox::TimeHelper::now().time_since_epoch()).count();
    std::string folder = "DV_" + std::to_string(now_ns);

    redirect_path_ = toolbox::StringHelper::joinPath(config_->sim_temporary_redirect_path_, folder);
    std::cout << "Creating redirect folder " << redirect_path_ << std::endl;
    std::string command = "mkdir -p " + redirect_path_;
    system(command.c_str());

    return toolbox::FileSystemHelper::folderExists(redirect_path_);
}

void DV::removeRedirectFolder() {
    assert(!redirect_path_.empty() && toolbox::FileSystemHelper::folderExists(redirect_path_));

    std::cout << std::endl << "Removing redirect folder " << redirect_path_ << std::endl;
    std::string command = "rm -r " + redirect_path_;
    system(command.c_str());
}

int DV::startServerPart(const std::string &port) {
    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // current use as before: use provided IP address in host_

    if ((status = getaddrinfo(config_->dv_hostname_.c_str(), port.c_str(), &hints, &servinfo)) != 0) {
        std::cerr << "getaddrinfo error: " << gai_strerror(status);
        exit(1);
    }

    int sock = socket(servinfo->ai_family, servinfo->ai_socktype, 0 /* servinfo->ai_protocol */);
    if (sock < 0) {
        std::cerr << "socket error: " << errno;
        exit(1);
    }

    int yes = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
        std::cerr << "setsockopt error: " << errno;
        exit(1);
    }

    if (bind(sock, servinfo->ai_addr, servinfo->ai_addrlen) < 0) {
        std::cerr << "binding error: " << errno;
        exit(1);
    }

    freeaddrinfo(servinfo);

    if (listen(sock, kListenBacklog) == -1) {
        std::cerr << "start listening error: " << errno;
        exit(1);
    }

    if (config_->dv_debug_output_on_) {
        std::cout << "started server " << config_->dv_hostname_ << ":" << port << " with socket " << sock << std::endl;
    }

    return sock;
}

void DV::startServer() {
    sim_socket_ = startServerPart(config_->dv_sim_port_);
    client_socket_ = startServerPart(config_->dv_client_port_);
    std::cout << std::endl << "DV server online. simulator: " << config_->dv_hostname_ << ":" << config_->dv_sim_port_
              << ", client: " << config_->dv_hostname_ << ":" << config_->dv_client_port_ << std::endl
              << "dv_max_prefetching_intervals " << config_->dv_max_prefetching_intervals_
              << ", dv_max_parallel_simjobs " << config_->dv_max_parallel_simjobs_ << std::endl;
}

void DV::stopServer() {
    close(client_socket_);
    close(sim_socket_);
    std::cout << "DV server sockets for simulator and client closed." << std::endl << std::endl;
}

void DV::printStats() {
    stats_.print(&std::cout);
}

void DV::printAccessTrace() {
    std::cout << std::endl << std::endl << "Access trace" << std::endl;
    std::vector<dv::id_type> trace;
    dv::id_type max_id = simulator_ptr_->convertTrace(&trace, filecache_ptr_->getAccessTrace());
    dv::id_type n = trace.size();
    filecache_ptr_->printStatus(&std::cout);
    std::cout << "n_access       = " << n << std::endl
              << "max_value      = " << max_id << std::endl
              << "alpha (median) = " << simulator_ptr_->getAlpha() << std::endl
              << "tau (median)   = " << simulator_ptr_->getTau() << std::endl
              << std::endl;

    // detailed descriptive statistics on collected alpha and tau
    // (more informative logging than just median)
    toolbox::StatisticsHelper::calcAndPrintSummary(&std::cout, "alpha", simulator_ptr_->getAlphas());
    toolbox::StatisticsHelper::calcAndPrintSummary(&std::cout, "tau", simulator_ptr_->getTaus());

    std::cout << std::endl << "access_list    = [" << std::endl;
    dv::id_type counter = 0;
    for (const auto &item : trace) {
        ++counter;
        if (counter % 20 == 0) {
            std::cout << std::endl;
        }
        std::cout << item;
        if (counter < n) {
            std::cout << ", ";
        }
    }
    std::cout << "]" << std::endl << std::endl;
}

void DV::finish() {
    stopServer();
    printStats();
    printAccessTrace();
    removeRedirectFolder();
}

}
