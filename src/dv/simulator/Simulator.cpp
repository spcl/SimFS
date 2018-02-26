//
// Created by Pirmin Schmid on 12.04.17.
//

#include <cmath>
#include <iostream>
#include <iomanip>
#include <sstream>

#include "Simulator.h"
#include "../server/DV.h"
#include "../caches/RestartFiles.h"
#include "../toolbox/FileSystemHelper.h"
#include "../toolbox/StatisticsHelper.h"
#include "../toolbox/StringHelper.h"


namespace dv {

Simulator::Simulator(DV *dv_ptr) : dv_ptr_(dv_ptr) {}

void Simulator::incFilesCount(dv::counter_type increment) {
    files_ += increment;
}

dv::counter_type Simulator::getFilesCount() const {
    return files_;
}

void Simulator::incSimulationsCount() {
    ++simulations_;
}

dv::counter_type Simulator::getSimulationsCount() const {
    return simulations_;
}

void Simulator::scanRestartFiles() {
    RestartFiles restart_files;

    Simulator *local_simulator_ptr = this;
    auto accept_function = [&] (const std::string &filename) -> bool {
        return local_simulator_ptr->getCheckpointFileType(filename) != 0;
    };

    auto add_file = [&] (const std::string &name,
                         const std::string &rel_path,
    const std::string &full_path) -> void {

        std::unique_ptr<FileDescriptor> fd = std::make_unique<FileDescriptor>(name, full_path);
        fd->setFileAvailable(true);
        dv::size_type size = toolbox::FileSystemHelper::fileSize(full_path);
        if (size < 0) {
            std::cerr << "scanRestartFiles: Could not determine file size of " << full_path << std::endl;
            size = 0;
        }
        fd->setSize(size);
        restart_files.put(name, std::move(fd));
    };

    toolbox::FileSystemHelper::readDir(dv_ptr_->getConfigPtr()->sim_checkpoint_path_,
                                       add_file, true, accept_function);

    std::vector<FileDescriptor *> files = restart_files.getAllFiles();

    std::cout  << "Simulator: " << files.size() << " restart files detected. Building lookup trees..." << std::endl;

    for (auto fd : files) {

        dv::id_type nr = checkpoint2nr(fd->getName());

        if (dv_ptr_->getConfigPtr()->dv_debug_output_on_) {
            std::cout << "restart file: " << fd->getName() << "; nr: " << nr << std::endl;
        }

        restarts_.emplace(nr);
        restarts_ceil_.emplace(nr);
    }

    std::cout << "restart lookup tree: " << restarts_.size()
              << ", ceil lookup tree: " << restarts_ceil_.size() << std::endl;

    if (dv_ptr_->getConfigPtr()->dv_debug_output_on_) {
        std::cout << "restart numbers: ";
        for (auto it : restarts_) {
            std::cout << it << " ";
        }
        std::cout << std::endl << "ceiling numbers: ";
        for (auto it : restarts_ceil_) {
            std::cout << it << " ";
        }
        std::cout << std::endl;
    }
}


//--- file predicates & file/id_type conversion functions ------------------

dv::id_type Simulator::getCheckpointFileType(const std::string &filename) const {
    return dv_ptr_->getConfigPtr()->get_checkpoint_file_type(filename);
}

dv::id_type Simulator::getResultFileType(const std::string &filename) const {
    return dv_ptr_->getConfigPtr()->get_result_file_type(filename);
}

dv::id_type Simulator::checkpoint2nr(const std::string &name, id_type file_type) const {
    return dv_ptr_->getConfigPtr()->checkpoint2nr(name, file_type);
}

dv::id_type Simulator::result2nr(const std::string &name, id_type file_type) const {
    return dv_ptr_->getConfigPtr()->result2nr(name, file_type);
}

dv::id_type Simulator::compare(const std::string &filename1, const std::string &filename2) const {
    dv::id_type t1 = result2nr(filename1);
    dv::id_type t2 = result2nr(filename2);
    return t2 - t1;
}


dv::id_type Simulator::getCheckpointNr(dv::id_type nr) const {
    auto it = restarts_.lower_bound(nr);
    dv::id_type checkpoint_nr = (it == restarts_.end()) ? 0 : *it;

    return checkpoint_nr;
}

dv::id_type Simulator::getNextCheckpointNr(dv::id_type nr) const {
    auto it = restarts_ceil_.lower_bound(nr);
    dv::id_type pstop = (it == restarts_ceil_.end()) ? nr : *it;

    return pstop;
}


dv::id_type Simulator::getPrevRestartDiff(const std::string &filename) const {
    dv::id_type nr = result2nr(filename);
    dv::id_type checkpoint_nr = getCheckpointNr(nr);
    return nr - checkpoint_nr;
}

dv::id_type Simulator::getNextRestartDiff(const std::string &filename) const {
    dv::id_type nr = result2nr(filename);
    dv::id_type pstop = getNextCheckpointNr(nr);
    return pstop - nr;
}


dv::id_type Simulator::convertTrace(std::vector<dv::id_type> *output,
                                    const std::vector<std::string> &input) const {
    dv::id_type max_id = 0;
    output->clear();
    for (const auto &name : input) {
        dv::id_type id = result2nr(name);
        output->push_back(id);
        if (max_id < id) {
            max_id = id;
        }
    }
    return max_id;
}

dv::id_type Simulator::partitionKey(const std::string &filename) const {
    dv::id_type nr = result2nr(filename);
    return getCheckpointNr(nr);
}

dv::cost_type Simulator::getCost(const std::string &filename) const {
    return getPrevRestartDiff(filename);
}

std::string Simulator::getMetadataFilename(const std::string &filename) const {
    std::string subdir = toolbox::FileSystemHelper::getDirname(filename);
    std::string metadir = toolbox::StringHelper::joinPath(dv_ptr_->getConfigPtr()->sim_result_path_, subdir);
    return toolbox::StringHelper::joinPath(metadir, ".meta");
}


//--- job generation -------------------------------------------------------

std::unique_ptr<SimJob> Simulator::generateSimJob(dv::id_type appid,
        dv::id_type target_nr,
        std::unique_ptr<toolbox::KeyValueStore> parameters) {
    // target_nr typically matches the filename that is in parameters
    // it may differ for prefetch target_nrs

    // generates a job for target_nr from nearest restart file up to the next one
    // (or target_nr if no restart file available)

    return generateSimJobForRange(appid, target_nr, target_nr, std::move(parameters));
}

std::unique_ptr<SimJob> Simulator::generateSimJobForRange(dv::id_type appid,
        dv::id_type target_start_nr,
        dv::id_type target_stop_nr,
        std::unique_ptr<toolbox::KeyValueStore> parameters) {
    // target_start_nr typically matches the filename that is in parameters
    // it may differ for prefetch target_nrs

    // generates a job for target_start_nr from nearest restart file up to the next one following target_stop_nr
    // (or target_stop_nr if no restart file available)

    std::unique_ptr<SimJob> simjob = std::make_unique<SimJob>(dv_ptr_, appid, target_start_nr, std::move(parameters));

    auto it1 = restarts_.lower_bound(target_start_nr);
    dv::id_type simstart = (it1 == restarts_.end()) ? 0 : *it1;

    // assure that full interval is simulated up to the next available checkpoint file
    // improved version that uses the effectively available checkpoint files and not only
    // the defined restart interval (if possible)
    auto it2 = restarts_ceil_.upper_bound(target_stop_nr);
    dv::id_type simstop = (it2 == restarts_ceil_.end()) ? target_stop_nr : *it2;

    // adjust simstop to exclude the actual next restart number (no overlap)
    // but do not go below target_end_nr
    simstop -= 1;
    if (simstop < target_stop_nr) {
        simstop = target_stop_nr;
    }

    simjob->setSimStart(simstart);
    simjob->setSimStop(simstop);


    LOG(CLIENT, 1, "SimJob: target: " + std::to_string(target_start_nr) + "->" + std::to_string(target_stop_nr) + "; actual values: " + std::to_string(simstart) + "->" + std::to_string(simstop));

    // continue with valid jobs
    if (dv_ptr_->getConfigPtr()->dv_debug_output_on_) {
        simjob->printParameters(&std::cout);
    }

    if (dv_ptr_->getConfigPtr()->optional_dv_prefetch_all_files_at_once_) {
        simstart = 0;
        simstop = dv_ptr_->getConfigPtr()->optional_sim_max_nr_;
        simjob->setSimStart(simstart);
        simjob->setSimStop(simstop);
        if (dv_ptr_->getConfigPtr()->dv_debug_output_on_) {
            LOG(CLIENT, 0, "Simulating everything! (optional_dv_prefetch_all_files_at_once): 0->" + std::to_string(simstop));
        }
    }

    return simjob;
}


//--- alpha and taus ---------------------------------------------------

void Simulator::addAlphaAndTau(double alpha, double tau) {
    if (alpha < 0.0 || tau < 0.0) {
        return;
    }
    alphas_.push_back(alpha);
    taus_.push_back(tau);
}

void Simulator::addAlpha(double alpha) {
    if (alpha < 0) {
        return;
    }
    alphas_.push_back(alpha);
}

void Simulator::extendAlphas(const std::vector<double> &alphas) {
    alphas_.insert(alphas_.end(), alphas.begin(), alphas.end());
}

const std::vector<double> &Simulator::getAlphas() const {
    return alphas_;
}

double Simulator::getAlpha() const {
    if (alphas_.size() == 0) {
        return -1.0;
    }

    return toolbox::StatisticsHelper::median(alphas_);
}

void Simulator::addTau(double tau) {
    if (tau < 0.0) {
        return;
    }
    taus_.push_back(tau);
}

void Simulator::extendTaus(const std::vector<double> &taus) {
    taus_.insert(taus_.end(), taus.begin(), taus.end());
}

const std::vector<double> &Simulator::getTaus() const {
    return taus_;
}

double Simulator::getTau() const {
    if (taus_.size() == 0) {
        return -1.0;
    }

    return toolbox::StatisticsHelper::median(taus_);
}

const toolbox::KeyValueStore &Simulator::getStatusSummary() {
    // update summary
    statusSummary_.setInt("sim_checkpoint_files", restarts_.size());
    statusSummary_.setInt("sim_median_alpha", getAlpha());
    statusSummary_.setDouble("sim_median_tau", getTau());

    // return
    return statusSummary_;
}
}
