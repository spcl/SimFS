//
// Created by Pirmin Schmid on 18.04.17.
//

#include "FileCacheLRU.h"

#include <iostream>
#include <cerrno>
#include <cstring>

#include "../../server/DV.h"
#include "../../simulator/Simulator.h"
#include "../../toolbox/FileSystemHelper.h"


namespace dv {

constexpr char FileCacheLRU::kCacheName[];

FileCacheLRU::FileCacheLRU(DV *dv_ptr, ID_type capacity)
    : FileCache(), dv_ptr_(dv_ptr), cache_name_(kCacheName),
      capacity_(capacity), protected_mrus_(1), cache_(capacity) {

    debug_messages_ = dv_ptr_->getConfigPtr()->filecache_debug_output_on_;
    cache_.setDebugMode(dv_ptr_->getConfigPtr()->filecache_details_debug_output_on_);
}

void FileCacheLRU::initializeWithFiles() {
    std::cout << cache_name_ << "initialized with capacity " << capacity_
              << " and protected MRUs " << protected_mrus_ << std::endl;

    Simulator *local_simulator_ptr = dv_ptr_->getSimulatorPtr();
    auto accept_function = [&](const std::string &filename) -> bool {
        return local_simulator_ptr->getResultFileType(filename) != 0;
    };

    auto add_file = [&] (const std::string &name,
                         const std::string &rel_path,
    const std::string &full_path) -> void {

        //std::cout << "LRU adding " << name << " (" << rel_path << ")" << std::endl;
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

    std::cout  << cache_name_ << cache_.size() << " files cached." << std::endl;
}

void FileCacheLRU::put(const std::string &key, std::unique_ptr<FileDescriptor> value) {
    if (debug_messages_) {
        std::cout << cache_name_ << "put_before: " << std::endl;
        printStatus(&std::cout);
    }

    dv::cost_type cost = dv_ptr_->getSimulatorPtr()->getCost(value->getName());
    value->setCost(cost);

    // protect against double insertions (should not happen if code is correct)
    ID_type cache_id = cache_.find(key);
    auto waiting_it = waiting_.find(key);
    if (!(cache_id == kNone && waiting_it == waiting_.end())) {
        std::cerr << "ERROR in " << cache_name_ << "key " << key << " already exists. Check usage of put() in client code." << std::endl;
        std::cerr << "NO change was made to the cache database!" << std::endl;

        std::cout << "ERROR in " << cache_name_ << "key already exists. Check usage of put() in client code." << std::endl;
        std::cout << "NO change was made to the cache database!" << std::endl;

        // any change would only introduces consecutive additional errors.
        // the client code MUST be changed to proper use of the cache in this case
        return;
    }

    // regular case
    if (value->isFileAvailable()) {
        actualPut(key, std::move(value));
    } else {
        waiting_[key] = std::move(value);
        if (debug_messages_) {
            std::cout << key << ": file is not yet available -> added to waiting list" << std::endl;
        }
    }
    // no further access to value allowed after here

    if (debug_messages_) {
        std::cout << cache_name_ << "put_after: " << std::endl;
        std::cout << "inserted file with key " << key;
        if (isCostAware()) {
            std::cout << " has cost " << cost << " if accessed by clients, or reduced cost otherwise" << std::endl;
        } else {
            std::cout << " has fixed unit cost " << std::endl;
        }
        printStatus(&std::cout);
    }
}

FileDescriptor *FileCacheLRU::get(const std::string &key) {
    if (debug_messages_) {
        std::cout << cache_name_ << "get: " << std::endl;
        printStatus(&std::cout);
    }

    access_trace_.push_back(key);
    FileDescriptor *d = actualGet(key);

    if (d == nullptr) {
        auto it = waiting_.find(key);
        if (it == waiting_.end()) {
            return nullptr;
        }
        return it->second.get();
    }

    // key found in cache

    // sanity check
    auto it = waiting_.find(key);
    if (it != waiting_.end()) {
        std::cerr << "ERROR in " << cache_name_ << "key " << key
                  << " exists in parallel in cache and waiting list. Check usage of put() and refresh()."
                  << std::endl;
    }

    return d;
}

FileDescriptor *FileCacheLRU::internal_lookup_get(const std::string &key) {
    ID_type id = cache_.find(key);
    if (id == kNone) {
        auto it = waiting_.find(key);
        if (it == waiting_.end()) {
            return nullptr;
        }
        return it->second.get();
    }

    return cache_.get(id)->get();
}

void FileCacheLRU::refresh(const std::string &key) {
    if (debug_messages_) {
        std::cout << cache_name_ << "refresh_before: " << std::endl;
        printStatus(&std::cout);
    }

    FileDescriptor *c = nullptr;
    ID_type c_id = cache_.find(key);
    if (c_id != kNone) {
        c = cache_.get(c_id)->get();
    }

    FileDescriptor *w = nullptr;
    auto w_it = waiting_.find(key);
    if (w_it != waiting_.end()) {
        w = w_it->second.get();
    }

    // case distinction for all theoretically possible cases
    // however, only 3 typical case should actually happen
    // 1) descriptor was in cache only
    //   -> just gets a refresh (move to MRU)
    // 2) descriptor was in waiting list and fileAvailable() still false
    //   -> nothing
    // 3) descriptor is in waiting list and fileAvailable() is true
    //   -> move to cache using actualPut() including potential eviction
    //      of former content
    // In all other cases, in particular if found in both or no locations
    // -> some form of error caused by wrong use of this cache library.

    if (c != nullptr) {
        if (w == nullptr) {
            if (c->isFileAvailable()) {
                // case 1) -> simple refresh
                cache_.refreshWithId(c_id);
            } else {
                // problem case: fileAvailable flag should not just get lost
                std::cerr << "ERROR in " << cache_name_ << "refresh: fileAvailable flag was removed manually in " << key
                          << ". This should not be done. Use the eviction mechanism to clear files." << std::endl;
                return;
            }
        } else {
            // problem case: found in cache and waiting list
            std::cerr << "ERROR in " << cache_name_ << "refresh: key " << key
                      << " can be found in BOTH, in cache and in waiting list. Check cache client code." << std::endl;
            return;
        }

    } else {
        // c == nullptr
        if (w != nullptr) {
            if (w->isFileAvailable()) {
                // case 3) -> move from waiting list to cache
                // additionally check whether file is really available
                if (toolbox::FileSystemHelper::fileExists(w->getFileName())) {
                    if (debug_messages_) {
                        std::cout << "   refresh triggers an actualPut() since file is available now." << std::endl;
                    }
                    actualPut(key, std::move(waiting_[key]));
                    waiting_.erase(key);

                } else {
                    std::cerr << "ERROR in " << cache_name_ << "refresh: key " << key
                              << " is said to exist but the file cannot be found. Check cache client code." << std::endl;
                    return;
                }
            } else {
                // case 2) ok. -> nothing needed
            }

        } else {
            // problem case: neither found in cache nor in waiting list
            std::cerr << "ERROR in " << cache_name_ << "refresh: key " << key
                      << " cannot be found: not in cache, not in waiting list. Check cache client code." << std::endl;
            return;
        }
    }

    if (debug_messages_) {
        std::cout << cache_name_ << "refresh_after: " << std::endl;
        printStatus(&std::cout);
    }
}

const std::vector<std::string> &FileCacheLRU::getAccessTrace() const {
    return access_trace_;
}

const std::string &FileCacheLRU::name() const {
    return cache_name_;
}

dv::id_type FileCacheLRU::capacity() const {
    return cache_.capacity();
}

dv::id_type FileCacheLRU::size() const {
    return cache_.size();
}

FileCollection::Stats FileCacheLRU::getStats() const {
    // note: this lruPredicate must be identical to the one in findVictim_LRU()
    auto lruPredicate = [](const std::unique_ptr<FileDescriptor> &fd) -> bool {
        return fd->getLockCount() == 0 && !fd->isFileUsedBySimulator();
    };

    cache_map_type::ID_type max_index = cache_.size() - 1;
    cache_map_type::map_type<Stats> mapFunction =
    [&](const std::unique_ptr<FileDescriptor> &fd, cache_map_type::ID_type index) -> Stats {
        const auto size = fd->getSize();
        const bool evictable = lruPredicate(fd) && index < max_index;
        Stats result = {
            1,
            evictable ? 1 : 0,
            size,
            evictable ? size : 0
        };
        return result;
    };

    cache_map_type::reduce_type<Stats, Stats> reduceFunction =
    [](const Stats &accumulator, const Stats &value) -> Stats {
        Stats result = {
            accumulator.count_all + value.count_all,
            accumulator.count_evictable + value.count_evictable,
            accumulator.filesize_all + value.filesize_all,
            accumulator.filesize_evictable + value.filesize_evictable
        };
        return result;
    };

    Stats zero = {};
    return cache_.mapreduce(zero, reduceFunction, mapFunction);
}

void FileCacheLRU::printStatus(std::ostream *out) {
    const Stats stats = getStats();
    *out << cache_name_ << " capacity " << cache_.capacity()
         << ", size " << stats.count_all << ", evictable " << stats.count_evictable
         << " (" << (stats.count_all > 0 ? ((stats.count_evictable * 100) / stats.count_all) : 0)
         << "%), waiting list size " << waiting_.size() << std::endl
         << "file size all " << stats.filesize_all
         << " bytes, evictable " << stats.filesize_evictable << " bytes"
         << ", " << (stats.filesize_all > 0 ? ((stats.filesize_evictable * 100) / stats.filesize_all) : 0)
         << "%" << std::endl;

    if (0 < cache_.size()) {
        ID_type id = cache_.getLruId();
        *out << "    LRU ID " << id << ": " << cache_.get(id)->get()->getName() << std::endl;
        id = cache_.getMruId();
        *out << "    MRU ID " << id << ": " << cache_.get(id)->get()->getName() << std::endl;
    }

    *out << std::endl;
}

const toolbox::KeyValueStore &FileCacheLRU::getStatusSummary() {
    // update summary
    const Stats stats = getStats();
    statusSummary_.setString("cache_name", cache_name_);
    statusSummary_.setInt("cache_capacity", cache_.capacity());
    statusSummary_.setInt("cache_size", stats.count_all);
    statusSummary_.setInt("cache_size_evictable", stats.count_evictable);
    statusSummary_.setInt("cache_size_evictable_percent", stats.count_all > 0 ? ((stats.count_evictable * 100) / stats.count_all) : 0);
    statusSummary_.setInt("cache_waiting", waiting_.size());
    statusSummary_.setInt("cache_filesize_all", stats.filesize_all);
    statusSummary_.setInt("cache_filesize_evictable", stats.filesize_evictable);
    statusSummary_.setInt("cache_filesize_evictable_percent", stats.filesize_all > 0 ? ((stats.filesize_evictable * 100) / stats.filesize_all) : 0);
    return statusSummary_;
}

//--- protected: internal API

void FileCacheLRU::actualPut(const std::string &key, std::unique_ptr<FileDescriptor> value) {
    if (cache_.size() < capacity_) {
        cache_.add(key, std::move(value));
    } else {
        id_cost_pair_type r = evict();
        if (r.first == kNone) {
            // no eviction could be made, error message was already printed
            // expand capacity to keep going
            cache_.add(key, std::move(value));
        } else {
            cache_.replace(r.first, key, std::move(value));
            // nothing is done with cost in LRU
        }
    }
}

FileDescriptor *FileCacheLRU::actualGet(const std::string &key) {
    ID_type id = cache_.find(key);
    if (id == kNone) {
        return nullptr;
    }

    return cache_.get(id)->get();
}

/**
 * due to performance reasons, this method does not actually perform the eviction in the cache system
 * but returns the id and cost back to the caller (i.e. actualPut), which will then use the ID to run
 * a replace() operation on the cache. However, the file is removed in file system.
 */
FileCacheLRU::id_cost_pair_type FileCacheLRU::evict() {
    id_cost_pair_type r = findVictim();
    if (r.first == kNone) {
        std::cerr << cache_name_
                  << "could not evict a file from cache. Too many files are open/locked at the moment for given cache capacity "
                  << capacity_ << ". Capacity will be increased to continue with service. Start service with larger capacity."
                  << std::endl;
        r.second = kCostForNone;
        return r;
    }

    // remove file
    FileDescriptor *descriptor = cache_.get(r.first)->get();
    dv_ptr_->getStatsPtr()->incEvictions(descriptor->getName());


    if (toolbox::FileSystemHelper::fileExists(descriptor->getFileName())) {
        toolbox::FileSystemHelper::rmFile(descriptor->getFileName());
        if (debug_messages_) {
            std::cout << cache_name_ << "removed file " << descriptor->getFileName() << std::endl;
        }
    } else {
        std::cerr << cache_name_ << "evict(): file " << descriptor->getFileName() << " should be removed, but does not exist. (" << std::strerror(errno) << ")" << std::endl;
        // continue, file is already "not there"
    }

    // eviction by replacement happens for the cache database in actualPut()
    return r;
}

FileCacheLRU::id_cost_pair_type FileCacheLRU::findVictim() {
    return findVictim_LRU();
}

bool FileCacheLRU::isCostAware() const {
    return kCostAware;
}

bool FileCacheLRU::isPartitionAware() const {
    return kPartitionAware;
}

/**
 * LRU is also used as a fallback mechanism -> never considers reserved MRUs
 * thus: default 1 == protecting the MRU itself, which is never evicted.
 */
FileCacheLRU::id_cost_pair_type FileCacheLRU::findVictim_LRU() {
    auto lruPredicate = [](const std::unique_ptr<FileDescriptor> &fd) -> bool {
        return fd->getLockCount() == 0 && !fd->isFileUsedBySimulator();
    };

    ID_type victim = cache_.findFirstWithPredicate(lruPredicate, false, 1);
    if (victim == kNone) {
        return {kNoneWorkaround, kCostForNone};
    }

    return {victim, kCostForLRU};
}

}
