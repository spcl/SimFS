//
// Created by Pirmin Schmid on 18.04.17.
//

#include "FileCacheUnlimited.h"

#include <iostream>

#include "../../server/DV.h"
#include "../../simulator/Simulator.h"
#include "../../toolbox/FileSystemHelper.h"

namespace dv {
constexpr char FileCacheUnlimited::kCacheName[];

FileCacheUnlimited::FileCacheUnlimited(DV *dv_ptr) : dv_ptr_(dv_ptr), cache_name_(kCacheName) {}

void FileCacheUnlimited::initializeWithFiles() {
    Simulator *local_simulator_ptr = dv_ptr_->getSimulatorPtr();
    auto accept_function = [&](const std::string &filename) -> bool {
        return local_simulator_ptr->getResultFileType(filename) != 0;
    };

    auto add_file = [&] (const std::string &name,
                         const std::string &rel_path,
    const std::string &full_path) -> void {

        std::unique_ptr<FileDescriptor> fd = std::make_unique<FileDescriptor>(name, full_path);
        fd->setFileAvailable(true);
        dv::size_type size = toolbox::FileSystemHelper::fileSize(full_path);
        if (size < 0) {
            std::cerr << "FileCache: Could not determine file size of " << full_path << std::endl;
            size = 0;
        }
        fd->setSize(size);
        this->put(rel_path, std::move(fd));
    };

    toolbox::FileSystemHelper::readDir(dv_ptr_->getConfigPtr()->sim_result_path_,
                                       add_file, true, accept_function);

    std::cout << "FileCacheUnlimited initialized: " << files_.size() << " files cached." << std::endl;
}

void FileCacheUnlimited::put(const std::string &key, std::unique_ptr<FileDescriptor> value) {
    total_file_size_ += value->getSize();
    files_[key] = std::move(value);
}

FileDescriptor *FileCacheUnlimited::get(const std::string &key) {
    access_trace_.push_back(key);
    return internal_lookup_get(key);
}

FileDescriptor *FileCacheUnlimited::internal_lookup_get(const std::string &key) {
    auto it = files_.find(key);
    if (it == files_.end()) {
        return nullptr;
    }

    return it->second.get();
}

void FileCacheUnlimited::refresh(const std::string &key) {
    // nothing
}

const std::vector<std::string> &FileCacheUnlimited::getAccessTrace() const {
    return access_trace_;
}

const std::string &FileCacheUnlimited::name() const {
    return cache_name_;
}

dv::id_type FileCacheUnlimited::capacity() const {
    return -1; // unlimited (i.e. limited by file system and max of dv::id_type)
}

dv::id_type FileCacheUnlimited::size() const {
    return files_.size();
}

FileCollection::Stats FileCacheUnlimited::getStats() const {
    // note: this simulates a cache that is always empty, i.e. has unlimited additional capacity.
    Stats result = {static_cast<dv::counter_type>(files_.size()),
                    static_cast<dv::counter_type>(files_.size()),
                    total_file_size_,
                    total_file_size_
                   };
    return result;
}

void FileCacheUnlimited::printStatus(std::ostream *out) {
    *out << "Unlimited cache: size " << files_.size()
         << ", total filesize " << total_file_size_ << std::endl;
}

const toolbox::KeyValueStore &FileCacheUnlimited::getStatusSummary() {
    // update summary
    statusSummary_.setString("cache_name", "unlimited");
    statusSummary_.setString("cache_capacity", "unlimited");
    statusSummary_.setInt("cache_size", files_.size());
    statusSummary_.setString("cache_size_evictable", "not applicable");
    statusSummary_.setInt("cache_filesize_all", total_file_size_);
    statusSummary_.setString("cache_filesize_evictable", "not applicable");
    return statusSummary_;
}
}
