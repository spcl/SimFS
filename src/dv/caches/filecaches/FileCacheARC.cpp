//
// Created by Pirmin Schmid on 21.09.17.
//

#include "FileCacheARC.h"

#include <cassert>
#include <cstring>

#include <iostream>

#include "../../server/DV.h"
#include "../../toolbox/FileSystemHelper.h"

namespace dv {

constexpr char FileCacheARC::kCacheName[];

FileCacheARC::FileCacheARC(DV *dv_ptr, FileCacheARC::ID_type capacity)
    : dv_ptr_(dv_ptr), T1_(capacity), T2_(capacity), B1_(capacity), B2_(capacity), capacity_(capacity),
      cache_name_(kCacheName) {

    debug_messages_ = dv_ptr_->getConfigPtr()->filecache_debug_output_on_;
    T1_.setDebugMode(dv_ptr_->getConfigPtr()->filecache_details_debug_output_on_);
    T2_.setDebugMode(dv_ptr_->getConfigPtr()->filecache_details_debug_output_on_);
    B1_.setDebugMode(dv_ptr_->getConfigPtr()->filecache_details_debug_output_on_);
    B2_.setDebugMode(dv_ptr_->getConfigPtr()->filecache_details_debug_output_on_);
}

void FileCacheARC::initializeWithFiles() {
    std::cout << cache_name_ << "initialized with capacity " << capacity_ << std::endl;

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
        this->put(name, std::move(fd));
    };

    toolbox::FileSystemHelper::readDir(dv_ptr_->getConfigPtr()->sim_result_path_,
                                       add_file, true, accept_function);

    std::cout  << cache_name_ << size() << " files cached." << std::endl;
}

void FileCacheARC::put(const std::string &key, std::unique_ptr<FileDescriptor> value) {
    if (debug_messages_) {
        std::cout << cache_name_ << "put_before: " << std::endl;
        printStatus(&std::cout);
    }

    dv::cost_type cost = dv_ptr_->getSimulatorPtr()->getCost(value->getName());
    value->setCost(cost);

    // protect against double insertions (should not happen if code is correct)
    // additional handling for cases where items are known in B1 or B2
    // -> replace FileDescriptor and handle in refresh()
    location_type location = find(key);

    // B1 or B2 are ok, but need to be handled specially
    if (location.map == map_type::kB1) {
        B1_.replace(location.id, key, std::move(value));
        refresh_internal(key, location);
        return;
    }

    if (location.map == map_type::kB2) {
        B2_.replace(location.id, key, std::move(value));
        refresh_internal(key, location);
        return;
    }

    // all the other locations, including waiting list, are not allowed

    if (location.map != map_type::kNotFound) {
        std::cerr << "ERROR in " << cache_name_ << "key already exists. Check usage of put() in client code." << std::endl;
        std::cerr << "NO change was made to the cache database!" << std::endl;

        std::cout << "ERROR in " << cache_name_ << "key already exists. Check usage of put() in client code." << std::endl;
        std::cout << "NO change was made to the cache database!" << std::endl;

        // any change would only introduces consecutive additional errors.
        // the client code MUST be changed to proper use of the cache in this case
        return;
    }

    // regular case
    if (value->isFileAvailable()) {
        handleCaseIV(key, location, std::move(value));
    } else {
        waiting_descriptor wd;
        wd.type = 0;
        wd.fd = std::move(value);
        waiting_[key] = std::move(wd);
        if (debug_messages_) {
            std::cout << key << ": file is not yet available -> added to waiting list (regular)" << std::endl;
        }
    }
    // no further access to value allowed after here

    if (debug_messages_) {
        std::cout << cache_name_ << "put_after: " << std::endl;
        std::cout << "inserted file with key " << key << std::endl;
        printStatus(&std::cout);
    }
}

FileDescriptor *FileCacheARC::get(const std::string &key) {
    if (debug_messages_) {
        std::cout << cache_name_ << "get: " << std::endl;
        printStatus(&std::cout);
    }

    access_trace_.push_back(key);

    return internal_lookup_get(key);
}

FileDescriptor *FileCacheARC::internal_lookup_get(const std::string &key) {
    location_type location = find(key);
    return location2ptr(location);
}

void FileCacheARC::refresh(const std::string &key) {
    if (debug_messages_) {
        std::cout << cache_name_ << "refresh_before: " << std::endl;
        printStatus(&std::cout);
    }

    location_type location = find(key);

    if (location.map == map_type::kNotFound) {
        std::cerr << cache_name_ << "ERROR in refresh(): key not found. check cache client code." << std::endl;
        return;
    }

    // note: B1 or B2 are not ok through refresh(); they would come through put()
    if (location.map == map_type::kB1) {
        std::cerr << cache_name_ << "ERROR in refresh(): key found in B1. check cache client code." << std::endl;
        return;
    }

    if (location.map == map_type::kB2) {
        std::cerr << cache_name_ << "ERROR in refresh(): key found in B2. check cache client code." << std::endl;
        return;
    }

    // all the other locations are ok

    refresh_internal(key, location);

    if (debug_messages_) {
        std::cout << cache_name_ << "refresh_after: " << std::endl;
        printStatus(&std::cout);
    }
}

const std::vector<std::string> &FileCacheARC::getAccessTrace() const {
    return access_trace_;
}

const std::string &FileCacheARC::name() const {
    return cache_name_;
}

dv::id_type FileCacheARC::capacity() const {
    return capacity_;
}

dv::id_type FileCacheARC::size() const {
    return T1_.size() + T2_.size();
}

FileCollection::Stats FileCacheARC::getStats() const {
    // note: this lruPredicate must be identical to the one in findVictim()
    auto lruPredicate = [](const std::unique_ptr<FileDescriptor> &fd) -> bool {
        return fd->getLockCount() == 0 && !fd->isFileUsedBySimulator();
    };

    cache_map_type::ID_type max_index1 = T1_.size() - 1;
    cache_map_type::map_type<Stats> mapFunction1 =
    [&](const std::unique_ptr<FileDescriptor> &fd, cache_map_type::ID_type index) -> Stats {
        const auto size = fd->getSize();
        const bool evictable = lruPredicate(fd) && index < max_index1;
        Stats result = {
            1,
            evictable ? 1 : 0,
            size,
            evictable ? size : 0
        };
        return result;
    };

    cache_map_type::ID_type max_index2 = T2_.size() - 1;
    cache_map_type::map_type<Stats> mapFunction2 =
    [&](const std::unique_ptr<FileDescriptor> &fd, cache_map_type::ID_type index) -> Stats {
        const auto size = fd->getSize();
        const bool evictable = lruPredicate(fd) && index < max_index2;
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
    Stats tmp = T1_.mapreduce(zero, reduceFunction, mapFunction1);
    return T2_.mapreduce(tmp, reduceFunction, mapFunction2);
}

void FileCacheARC::printStatus(std::ostream *out) {
    const Stats stats = getStats();
    *out << cache_name_ << " capacity " << capacity_
         << ", size " << stats.count_all << " (T1 target p " << p_ << ", T1 " << T1_.size()
         << "; T2 target (capacity - p) " << (capacity_ - p_) << ", T2 " << T2_.size()
         << "; B1 " << total_B1_size() << ", B2 " << total_B2_size()
         << "), evictable " << stats.count_evictable
         << " (" << (stats.count_all > 0 ? ((stats.count_evictable * 100) / stats.count_all) : 0)
         << "%), waiting list size " << waiting_count(0) << std::endl
         << "file size all " << stats.filesize_all
         << " bytes, evictable " << stats.filesize_evictable << " bytes"
         << ", " << (stats.filesize_all > 0 ? ((stats.filesize_evictable * 100) / stats.filesize_all) : 0)
         << "%" << std::endl;

    *out << std::endl;
}

const toolbox::KeyValueStore &FileCacheARC::getStatusSummary() {
    // update summary
    const Stats stats = getStats();
    statusSummary_.setString("cache_name", cache_name_);
    statusSummary_.setInt("cache_capacity", capacity_);
    statusSummary_.setInt("cache_size", stats.count_all);
    statusSummary_.setInt("cache_size_evictable", stats.count_evictable);
    statusSummary_.setInt("cache_size_evictable_percent", stats.count_all > 0 ? ((stats.count_evictable * 100) / stats.count_all) : 0);
    statusSummary_.setInt("cache_waiting", waiting_.size()); // including all kind of waiting (regular, B1, B2)
    statusSummary_.setInt("cache_filesize_all", stats.filesize_all);
    statusSummary_.setInt("cache_filesize_evictable", stats.filesize_evictable);
    statusSummary_.setInt("cache_filesize_evictable_percent", stats.filesize_all > 0 ? ((stats.filesize_evictable * 100) / stats.filesize_all) : 0);
    return statusSummary_;
}


//--- private functions ------------------------------------------------------------------------

FileCacheARC::ID_type FileCacheARC::total_B1_size() {
    return B1_.size() + waiting_count(1);
}

FileCacheARC::ID_type FileCacheARC::total_B2_size() {
    return B2_.size() + waiting_count(2);
}

FileCacheARC::ID_type FileCacheARC::waiting_count(int type) {
    ID_type count = 0;
    for (const auto &w : waiting_) {
        if (w.second.type == type) {
            ++count;
        }
    }
    return count;
}

FileCacheARC::location_type FileCacheARC::find(const std::string &key) {
    location_type location = { .map = map_type ::kNotFound, .id = kNone, .w_u_ptr = nullptr};

    // checks all 5 possible locations for sanity check (each file descriptor can only be at one location at most
    bool found = false;
    ID_type id = T1_.find(key);
    if (id != kNone) {
        location.map = map_type ::kT1;
        location.id = id;
        found = true;
    }

    id = T2_.find(key);
    if (id != kNone) {
        if (found) {
            std::cerr << cache_name_ << "ERROR in find(): file detected in T1 and T2. Check cache client code. "
                      << key << std::endl;
        } else {
            location.map = map_type::kT2;
            location.id = id;
            found = true;
        }
    }

    id = B1_.find(key);
    if (id != kNone) {
        if (found) {
            std::cerr << cache_name_ << "ERROR in find(): file detected in (T1 or T2) and B1. Check cache client code. "
                      << key << std::endl;
        } else {
            location.map = map_type::kB1;
            location.id = id;
            found = true;
        }
    }

    id = B2_.find(key);
    if (id != kNone) {
        if (found) {
            std::cerr << cache_name_ << "ERROR in find(): file detected in (T1 or T2 or B1) and B2. Check cache client code. "
                      << key << std::endl;
        } else {
            location.map = map_type::kB2;
            location.id = id;
            found = true;
        }
    }

    auto it = waiting_.find(key);
    if (it != waiting_.end()) {
        if (found) {
            std::cerr << cache_name_ << "ERROR in find(): file detected in (T1 or T2 or B1 or B2) and waiting. Check cache client code. "
                      << key << std::endl;
        } else {
            switch (it->second.type) {
            case 1:
                location.map = map_type::kWaitingB1;
                break;
            case 2:
                location.map = map_type::kWaitingB2;
                break;
            default:
                location.map = map_type::kWaitingRegular;
            }
            location.w_u_ptr = &(it->second.fd);
            found = true;
        }
    }

    return location;
}

/** handles both as B2, of course: kB2 und kWaitingB2 */
void FileCacheARC::replace(const FileCacheARC::location_type &xt) {
    ID_type t1_size = T1_.size();
    if ( (0 < t1_size)
            && (p_ < t1_size || ( (xt.map == kB2 || xt.map == kWaitingB2) && t1_size == p_)) ) {
        ID_type victim = findVictim(T1_);
        if (victim != kNone) {
            std::unique_ptr<FileDescriptor> v_ptr = std::move(*(T1_.get(victim)));
            T1_.erase(victim);
            evictFile(v_ptr.get());
            std::string key = v_ptr->getName();
            B1_.add(key, std::move(v_ptr));
        } else {
            std::cerr << "Warning: " << cache_name_ << "ERROR in replace(): no evictable file found in T1" << std::endl;
            // and just let it grow (nothing that could be done here)
        }
    } else {
        ID_type victim = findVictim(T2_);
        if (victim != kNone) {
            std::unique_ptr<FileDescriptor> v_ptr = std::move(*(T2_.get(victim)));
            T2_.erase(victim);
            evictFile(v_ptr.get());
            std::string key = v_ptr->getName();
            B2_.add(key, std::move(v_ptr));
        } else {
            std::cerr << "Warning: "<< cache_name_ << "ERROR in replace(): no evictable file found in T2" << std::endl;
            // and just let it grow (nothing that could be done here)
        }
    }
}

void FileCacheARC::evictFile(FileDescriptor *fd) {
    // the file must be evictable (no lock by client nor simulator)
    // assured that this file was just found by findVictim()

    // remove file
    dv_ptr_->getStatsPtr()->incEvictions(fd->getName());


    if (toolbox::FileSystemHelper::fileExists(fd->getFileName())) {
        toolbox::FileSystemHelper::rmFile(fd->getFileName());
        if (debug_messages_) {
            std::cout << cache_name_ << "removed file " << fd->getFileName() << std::endl;
        }
    } else {
        std::cerr << "Warning: " << cache_name_ << "evictFile(): file " << fd->getFileName() << " should be removed, but does not exist. (" << std::strerror(errno) << ")" << std::endl;
        // continue, file is already "not there"
    }

    fd->setFileAvailable(false);
}

FileCacheARC::ID_type FileCacheARC::findVictim(const FileCacheARC::cache_map_type &map) {
    auto lruPredicate = [](const std::unique_ptr<FileDescriptor> &fd) -> bool {
        return fd->getLockCount() == 0 && !fd->isFileUsedBySimulator();
    };

    return map.findFirstWithPredicate(lruPredicate, false, 0); // including the MRU in worst case
}


/** value->isFileAvailable() == true or false; called by refresh() and put() */
void FileCacheARC::refresh_internal(const std::string &key, const FileCacheARC::location_type &location) {
    assert(location.map != map_type::kNotFound);

    FileDescriptor *value = location2ptr_internal(location);
    if (value->isFileAvailable()) {
        handleUpdate(key, location);
    } else {
        // handle adjustments when coming from put
        // B1 -> waitingB1
        // B2 -> waitingB2
        // waitingRegular remains unchanged

        if (location.map == map_type::kB1) {
            waiting_descriptor wd;
            wd.type = 1;
            wd.fd = std::move(*(B1_.get(location.id)));
            B1_.erase(location.id);
            waiting_[key] = std::move(wd);
            if (debug_messages_) {
                std::cout << key << ": file is not yet available -> added to waiting list (B1)" << std::endl;
            }
        } else if (location.map == map_type::kB2) {
            waiting_descriptor wd;
            wd.type = 2;
            wd.fd = std::move(*(B2_.get(location.id)));
            B2_.erase(location.id);
            waiting_[key] = std::move(wd);
            if (debug_messages_) {
                std::cout << key << ": file is not yet available -> added to waiting list (B1)" << std::endl;
            }
        }
    }
}

/**
 * handles all 4 cases as described in Fig. 4
 *
 * location.map != map_type::kNotFound for all cases here
 *
 * value->isFileAvailable() == true for all cases
 *
 * note: for data management reasons
 * - case I has to distinguish between map_type::kT1 and kT2 (move or refresh)
 * - case IV has to distinguish between map_type::kWaiting and kNotFound (from put() directly to handleCaseIV() )
 */
void FileCacheARC::handleUpdate(const std::string &key, const FileCacheARC::location_type &location) {
    ID_type b1_size = 0;
    ID_type b2_size = 0;
    ID_type delta = 0;
    dv::id_type old_p = 0;
    std::unique_ptr<FileDescriptor> from;
    switch (location.map) {
    case map_type::kT1:
        // case I a: move from T1 to MRU in T2
        from = std::move(*(T1_.get(location.id)));
        T1_.erase(location.id);
        T2_.add(key, std::move(from));
        if (debug_messages_) {
            std::cout << "T2: inserted file from T1; key " << key << std::endl;
        }
        break;

    case map_type::kT2:
        // case I b: refresh to MRU in T2
        T2_.refreshWithId(location.id);
        break;

    case map_type::kB1:
    case map_type::kWaitingB1:
        // case II
        b1_size = total_B1_size();
        b2_size = total_B2_size();
        if (b1_size >= b2_size) {
            delta = 1;
        } else {
            // note b1_size is at least 1 -> no div/0; integer operations used here
            delta = b2_size / b1_size;
        }

        old_p = p_;
        p_ += delta;
        if (p_ > capacity_) {
            p_ = capacity_;
        }

        if (debug_messages_) {
            std::cout << cache_name_ << "handleUpdate(): case II with |B1|=" << b1_size
                      << ", |B2|=" << b2_size << ", old_p=" << old_p << ", new_p=" << p_
                      << ", key " << key << std::endl;
        }

        replace(location);

        // move from B1 to MRU in T2
        if (location.map == kB1) {
            from = std::move(*(B1_.get(location.id)));
            B1_.erase(location.id);
        } else {
            from = std::move(*(location.w_u_ptr));
            waiting_.erase(key);
        }

        T2_.add(key, std::move(from));
        if (debug_messages_) {
            std::cout << "T2: inserted file from B1; key " << key << std::endl;
        }

        // xt is already fetched into the cache
        break;

    case map_type::kB2:
    case map_type::kWaitingB2:
        // case III
        b1_size = total_B1_size();
        b2_size = total_B2_size();
        if (b2_size >= b1_size) {
            delta = 1;
        } else {
            // note b2_size is at least 1 -> no div/0; integer operations used here
            delta = b1_size / b2_size;
        }

        old_p = p_;
        p_ -= delta;
        if (p_ < 0) {
            p_ = 0;
        }

        if (debug_messages_) {
            std::cout << cache_name_ << "handleUpdate(): case III with |B1|=" << b1_size
                      << ", |B2|=" << b2_size << ", old_p=" << old_p << ", new_p=" << p_
                      << ", key " << key << std::endl;
        }

        replace(location);

        // move from B2 to MRU in T2
        if (location.map == kB2) {
            from = std::move(*(B2_.get(location.id)));
            B2_.erase(location.id);
        } else {
            from = std::move(*(location.w_u_ptr));
            waiting_.erase(key);
        }

        T2_.add(key, std::move(from));
        if (debug_messages_) {
            std::cout << "T2: inserted file from B2; key " << key << std::endl;
        }

        // xt is already fetched into the cache
        break;

    case map_type::kWaitingRegular:
        // case IV a: move from waiting to T1
        from = std::move(*(location.w_u_ptr));
        waiting_.erase(key);
        handleCaseIV(key, location, std::move(from));
        break;

    case map_type::kNotFound:
        // case IV b: insert into T1. put() calls handleCaseIV() directly
        std::cerr << cache_name_ << "ERROR in handleUpdate(): map_type::kNotFound should not happen here." << std::endl;
        break;

    default:
        std::cerr << cache_name_ << "ERROR in handleUpdate(): undefined map location." << std::endl;
    }
}

/**
 * used for put() and indirectly for refresh() after resolving waiting list
 */
void FileCacheARC::handleCaseIV(const std::string &key, const FileCacheARC::location_type &location,
                                std::unique_ptr<FileDescriptor> value) {

    assert(value->isFileAvailable());

    ID_type L1_size = T1_.size() + total_B1_size();
    if (debug_messages_) {
        std::cout << cache_name_ << "handleCaseIV(): L1 (=T1 + B1) size " << L1_size << std::endl;
    }

    if (L1_size >= capacity_) {
        // sub-case A in Fig 4 case IV
        // modified from == to >=
        if (T1_.size() < capacity_ && B1_.size() > 0) {
            // only descriptors but files in B1: remove of LRU without problem
            // just a security check to prevent calling erase on an empty B1_ linkdedmap
            B1_.erase(B1_.getLruId());
            replace(location);
        } else {
            // take care of files in T1
            ID_type victim = findVictim(T1_);
            if (victim != kNone) {
                std::unique_ptr<FileDescriptor> v_ptr = std::move(*(T1_.get(victim)));
                T1_.erase(victim);
                evictFile(v_ptr.get());
            } else {
                std::cerr << cache_name_ << "ERROR in handleCaseIV(): no evictable file found in T1" << std::endl;
                // and just let it grow (nothing that could be done here)
            }
        }
    } else {
        // sub-case B in Fig 4 case IV
        ID_type all_size = L1_size + T2_.size() + total_B2_size();
        if (all_size >= capacity_) {
            if (all_size >= (capacity_ + capacity_)) {
                // modified: used >= instead of only == for some safety
                // also an additional safety check that B2 is indeed not empty
                if (0 < B2_.size()) {
                    // here we check actual B2 size and not total B2 size
                    B2_.erase(B2_.getLruId());
                } else {
                    std::cout << cache_name_ << "WARNING in handleCaseIV(): B2 should not be empty here." << std::endl;
                    // and just let it pass
                }
            }

            replace(location);
        }
    }

    // add to MRU in T1
    T1_.add(key, std::move(value));
    if (debug_messages_) {
        std::cout << "T1: inserted file with key " << key << std::endl;
    }

    // xt already fetched
}

inline FileDescriptor *FileCacheARC::location2ptr(const FileCacheARC::location_type &location) {
    switch (location.map) {
    case map_type::kT1:
        return T1_.get(location.id)->get();
    case map_type::kT2:
        return T2_.get(location.id)->get();
    case map_type::kB1:
        return nullptr;
    case map_type::kB2:
        return nullptr;
    case map_type::kWaitingRegular:
    case map_type::kWaitingB1:
    case map_type::kWaitingB2:
        return location.w_u_ptr->get();
    default:
        return nullptr;
    }
}

inline FileDescriptor *FileCacheARC::location2ptr_internal(const FileCacheARC::location_type &location) {
    switch (location.map) {
    case map_type::kT1:
        return T1_.get(location.id)->get();
    case map_type::kT2:
        return T2_.get(location.id)->get();
    case map_type::kB1:
        return B1_.get(location.id)->get();;
    case map_type::kB2:
        return B2_.get(location.id)->get();;
    case map_type::kWaitingRegular:
    case map_type::kWaitingB1:
    case map_type::kWaitingB2:
        return location.w_u_ptr->get();
    default:
        return nullptr;
    }
}
}
