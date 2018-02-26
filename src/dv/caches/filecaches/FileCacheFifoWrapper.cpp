//
// Created by Pirmin Schmid on 22.09.17.
//

#include "FileCacheFifoWrapper.h"

#include <cstring>

#include "../../server/DV.h"
#include "../../toolbox/FileSystemHelper.h"

namespace dv {

constexpr char FileCacheFifoWrapper::kCacheName[];

FileCacheFifoWrapper::FileCacheFifoWrapper(DV *dv_ptr, std::unique_ptr<FileCache> embedded_cache,
        dv::id_type queue_capacity)
    : FileCache(), dv_ptr_(dv_ptr), embedded_cache_(std::move(embedded_cache)),
      fifo_queue_(queue_capacity) {

    debug_messages_ = dv_ptr_->getConfigPtr()->filecache_debug_output_on_;
    fifo_queue_.setDebugMode(dv_ptr_->getConfigPtr()->filecache_details_debug_output_on_);

    cache_name_ = std::string(kCacheName) + embedded_cache_->name();
}

void FileCacheFifoWrapper::initializeWithFiles() {
    embedded_cache_->initializeWithFiles();
}

void FileCacheFifoWrapper::put(const std::string &key, std::unique_ptr<FileDescriptor> value) {
    if (debug_messages_) {
        std::cout << cache_name_ << "put_before: " << std::endl;
        printStatus(&std::cout);
    }

    dv::cost_type cost = dv_ptr_->getSimulatorPtr()->getCost(value->getName());
    value->setCost(cost);

    // protect against double insertions (should not happen if code is correct)
    FileDescriptor *cache_ptr = embedded_cache_->internal_lookup_get(key);
    ID_type fifo_id = fifo_queue_.find(key);
    auto waiting_it = waiting_.find(key);
    if (!(cache_ptr == nullptr && fifo_id == fifo_queue_type::kNone && waiting_it == waiting_.end())) {
        std::cerr << "ERROR in " << cache_name_ << "key already exists. Check usage of put() in client code." << std::endl;
        std::cerr << "NO change was made to the cache database!" << std::endl;

        std::cout << "ERROR in " << cache_name_ << "key already exists. Check usage of put() in client code." << std::endl;
        std::cout << "NO change was made to the cache database!" << std::endl;

        // any change would only introduces consecutive additional errors.
        // the client code MUST be changed to proper use of the cache in this case
        return;
    }

    // regular case distinction as described in class description in header file
    if (value->isFileAvailable()) {
        if (0 < value->getUseCount()) {
            embedded_cache_->put(key, std::move(value));
        } else {
            fifoQueueAdd(key, std::move(value));
        }
    } else {
        waiting_[key] = std::move(value);
        if (debug_messages_) {
            std::cout << key << ": file is not yet available -> added to waiting list" << std::endl;
        }
    }
    // no further access to value allowed after here

    if (debug_messages_) {
        std::cout << cache_name_ << "put_after: " << std::endl;
        std::cout << "inserted file with key " << key << std::endl;
        printStatus(&std::cout);
    }
}

FileDescriptor *FileCacheFifoWrapper::get(const std::string &key) {
    if (debug_messages_) {
        std::cout << cache_name_ << "get: " << std::endl;
        printStatus(&std::cout);
    }

    access_trace_.push_back(key);

    // note: lookups in FIFO queue and waiting list are performed even if file was found in actual
    // cache to have ongoing sanity checks that cache items have not been introduced multiple times

    FileDescriptor *d = embedded_cache_->get(key);

    ID_type fifo_id = fifo_queue_.find(key);
    if (fifo_id != fifo_queue_type::kNone) {
        if (d == nullptr) {
            d = fifo_queue_.get(fifo_id)->get();
        } else {
            std::cerr << "ERROR in " << cache_name_ << "key " << key
                      << " exists in parallel in the embedded cache and FIFO queue. Check usage of put() and refresh()."
                      << std::endl;

            // use the descriptor from embedded cache
            return d;
        }
    }

    auto it = waiting_.find(key);
    if (it != waiting_.end()) {
        if (d == nullptr) {
            d = it->second.get();
        } else {
            std::cerr << "ERROR in " << cache_name_ << "key " << key
                      << " exists in parallel in the embedded cache / FIFO queue and waiting list. Check usage of put() and refresh()."
                      << std::endl;

            // use the descriptor from embedded cache / FIFO queue
            return d;
        }
    }

    return d;
}

FileDescriptor *FileCacheFifoWrapper::internal_lookup_get(const std::string &key) {
    FileDescriptor *c = embedded_cache_->internal_lookup_get(key);
    if (c != nullptr) {
        return c;
    }

    ID_type id = fifo_queue_.find(key);
    if (id != fifo_queue_type::kNone) {
        return fifo_queue_.get(id)->get();
    }

    auto it = waiting_.find(key);
    if (it == waiting_.end()) {
        return nullptr;
    }
    return it->second.get();
}

void FileCacheFifoWrapper::refresh(const std::string &key) {
    if (debug_messages_) {
        std::cout << cache_name_ << "refresh_before: " << std::endl;
        printStatus(&std::cout);
    }

    FileDescriptor *c = embedded_cache_->internal_lookup_get(key);

    FileDescriptor *f = nullptr;
    ID_type f_id = fifo_queue_.find(key);
    if (f_id != fifo_queue_type::kNone) {
        f = fifo_queue_.get(f_id)->get();
    }

    FileDescriptor *w = nullptr;
    auto w_it = waiting_.find(key);
    if (w_it != waiting_.end()) {
        w = w_it->second.get();
    }

    // sanity check: this simplifies the distinction tree below (not all conditional branches are needed)
    if ( !( (c != nullptr && f == nullptr && w == nullptr)
            || (c == nullptr && f != nullptr && w == nullptr)
            || (c == nullptr && f == nullptr && w != nullptr) ) ) {

        if (c == nullptr && f == nullptr && w == nullptr) {
            std::cerr << "ERROR during refresh() in " << cache_name_ << "key " << key
                      << " cannot be found in neither embedded cache, FIFO queue, nor waiting list. Check usage of put() and refresh()."
                      << std::endl;
        } else {
            std::cerr << "ERROR during refresh() in " << cache_name_ << "key " << key
                      << " exists in parallel in the embedded cache, FIFO queue and/or waiting list. Check usage of put() and refresh()."
                      << std::endl;
        }

        // nothing smart can actually be done at this time
        return;
    }


    // case distinctions as shown in detail in the class description in the header file

    if (c != nullptr) {
        // we know: f == nullptr, and w == nullptr

        if (c->isFileAvailable()) {
            // simple refresh
            embedded_cache_->refresh(key);
        } else {
            // problem case: fileAvailable flag should not just get lost
            std::cerr << "ERROR in " << cache_name_ << "refresh: fileAvailable flag was removed manually in " << key
                      << ". This should not be done. Use the eviction mechanism to clear files." << std::endl;
            return;
        }

    } else if (f != nullptr) {
        // we know: f_id is proper id, c == nullptr, and w == nullptr

        if (f->isFileAvailable()) {
            if (toolbox::FileSystemHelper::fileExists(f->getFileName())) {

                if (0 < f->getUseCount()) {
                    // file is now requested -> into embedded cache
                    if (debug_messages_) {
                        std::cout << " move from FIFO queue to embedded cache." << std::endl;
                    }
                    embedded_cache_->put(key, std::move(*fifo_queue_.get(f_id)));
                    fifo_queue_.erase(f_id);

                } else {
                    // not requested file -> ok. refresh within FIFO queue
                    fifo_queue_.refreshWithId(f_id);
                }

            } else {
                std::cerr << "ERROR in " << cache_name_ << "refresh: key " << key
                          << " is said to exist in FIFO queue but the file cannot be found. Check cache client code." << std::endl;
                return;
            }
        } else {
            std::cerr << "ERROR in " << cache_name_ << "refresh: key " << key
                      << " is said to exist in FIFO queue but the fileAvailable is false. Check cache client code." << std::endl;
            return;
        }

    } else {
        // we know: w != nullptr, c == nullptr, and f == nullptr

        if (w->isFileAvailable()) {
            if (toolbox::FileSystemHelper::fileExists(w->getFileName())) {

                if (0 < w->getUseCount()) {
                    // requested file -> into embedded cache
                    if (debug_messages_) {
                        std::cout << " available & requested file: move from waiting list to embedded cache." << std::endl;
                    }
                    embedded_cache_->put(key, std::move(waiting_[key]));

                } else {
                    // not requested file -> into FIFO queue
                    if (debug_messages_) {
                        std::cout << " available but not requested file: move from waiting list to FIFO queue." << std::endl;
                    }
                    fifoQueueAdd(key, std::move(waiting_[key]));
                }

                waiting_.erase(key);

            } else {
                std::cerr << "ERROR in " << cache_name_ << "refresh: key " << key
                          << " is said to exist but the file cannot be found. Check cache client code." << std::endl;
                return;
            }
        } else {
            // ok. -> nothing needed
        }
    }

    if (debug_messages_) {
        std::cout << cache_name_ << "refresh_after: " << std::endl;
        printStatus(&std::cout);
    }
}

const std::vector<std::string> &FileCacheFifoWrapper::getAccessTrace() const {
    return access_trace_;
}

const std::string &FileCacheFifoWrapper::name() const {
    return cache_name_;
}

dv::id_type FileCacheFifoWrapper::capacity() const {
    // -1 means unlimited
    dv::id_type embedded = embedded_cache_->capacity();
    if (embedded < 0) {
        return -1;
    }

    return fifo_queue_.capacity() + embedded;
}

dv::id_type FileCacheFifoWrapper::size() const {
    return fifo_queue_.size() + embedded_cache_->size();
}

FileCollection::Stats FileCacheFifoWrapper::getStats() const {
    FileCollection::Stats stats = embedded_cache_->getStats();

    // summarize FIFO queue
    auto lruPredicate = [](const std::unique_ptr<FileDescriptor> &fd) -> bool {
        return fd->getLockCount() == 0 && !fd->isFileUsedBySimulator();
    };

    fifo_queue_type::ID_type max_index = fifo_queue_.size() - 1;
    fifo_queue_type::map_type<Stats> mapFunction =
    [&](const std::unique_ptr<FileDescriptor> &fd, fifo_queue_type::ID_type index) -> Stats {
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

    fifo_queue_type::reduce_type<Stats, Stats> reduceFunction =
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
    FileCollection::Stats fifo = fifo_queue_.mapreduce(zero, reduceFunction, mapFunction);

    stats.count_all += fifo.count_all;
    stats.count_evictable += fifo.count_evictable;
    stats.filesize_all += fifo.filesize_all;
    stats.filesize_evictable += fifo.filesize_evictable;

    return stats;
}

void FileCacheFifoWrapper::printStatus(std::ostream *out) {
    const Stats stats = getStats();
    *out << cache_name_ << " capacity " << capacity()
         << " (FIFO queue " << fifo_queue_.capacity()
         << ", embedded cache " << embedded_cache_->capacity()
         << "), size " << stats.count_all << " (fifo " << fifo_queue_.size() << ", cache " << embedded_cache_->size()
         << "), evictable " << stats.count_evictable
         << " (" << (stats.count_all > 0 ? ((stats.count_evictable * 100) / stats.count_all) : 0)
         << "%), waiting list size " << waiting_.size() << std::endl
         << "file size all " << stats.filesize_all
         << " bytes, evictable " << stats.filesize_evictable << " bytes"
         << ", " << (stats.filesize_all > 0 ? ((stats.filesize_evictable * 100) / stats.filesize_all) : 0)
         << "%" << std::endl;

    *out << std::endl;
}

const toolbox::KeyValueStore &FileCacheFifoWrapper::getStatusSummary() {
    // update summary
    const Stats stats = getStats();
    statusSummary_.setString("cache_name", cache_name_);
    statusSummary_.setInt("cache_capacity", capacity());
    statusSummary_.setInt("cache_size", stats.count_all);
    statusSummary_.setInt("cache_size_evictable", stats.count_evictable);
    statusSummary_.setInt("cache_size_evictable_percent", stats.count_all > 0 ? ((stats.count_evictable * 100) / stats.count_all) : 0);
    statusSummary_.setInt("cache_waiting", waiting_.size());
    statusSummary_.setInt("cache_filesize_all", stats.filesize_all);
    statusSummary_.setInt("cache_filesize_evictable", stats.filesize_evictable);
    statusSummary_.setInt("cache_filesize_evictable_percent", stats.filesize_all > 0 ? ((stats.filesize_evictable * 100) / stats.filesize_all) : 0);
    return statusSummary_;
}


//--- private methods --------------------------------------------------------------------------

void FileCacheFifoWrapper::fifoQueueAdd(const std::string &key, std::unique_ptr<FileDescriptor> value) {
    if (fifo_queue_.size() < fifo_queue_.capacity()) {
        fifo_queue_.add(key, std::move(value));
    } else {
        // find a file that is not being overwritten at the moment by a simulator
        // note: while lock check is in here for completeness, no file in FIFO has a lock.
        // all files with locks (i.e. use count > 0) are in the embedded cache.

        auto lruPredicate = [](const std::unique_ptr<FileDescriptor> &fd) -> bool {
            return fd->getLockCount() == 0 && !fd->isFileUsedBySimulator();
        };

        ID_type victim = fifo_queue_.findFirstWithPredicate(lruPredicate, false, 0);

        if (victim == fifo_queue_type::kNone) {
            // this can only happen if all files in the FIFO queue are being re-simulated at the same time
            // which is very unlikely. Thus: report and silently extend FIFO queue by 1.
            // additional note: FIFO queue size should be at least as large as restart interval * number of parallel simulators + 1
            std::cout << cache_name_ << " no evictable file found in FIFO queue. All are being re-simulated at the same time."
                      << " Check FIFO queue size. Silent extension of FIFO queue size by 1." << std::endl;
            std::cerr << cache_name_ << " no evictable file found in FIFO queue. All are being re-simulated at the same time."
                      << " Check FIFO queue size. Silent extension of FIFO queue size by 1." << std::endl;

            fifo_queue_.add(key, std::move(value));
            return;
        }

        // remove file
        FileDescriptor *descriptor = fifo_queue_.get(victim)->get();
        dv_ptr_->getStatsPtr()->incFifoQueueEvictions(descriptor->getName());

        if (toolbox::FileSystemHelper::fileExists(descriptor->getFileName())) {
            toolbox::FileSystemHelper::rmFile(descriptor->getFileName());
            if (debug_messages_) {
                std::cout << cache_name_ << "removed file from FIFO queue " << descriptor->getFileName() << std::endl;
            }
        } else {
            std::cerr << cache_name_ << "evict(): file " << descriptor->getFileName() << " should be removed, but does not exist. (" << std::strerror(errno) << ")" << std::endl;
            // continue, file is already "not there"
        }

        // insert new descriptor
        fifo_queue_.replace(victim, key, std::move(value));
    }
}
}
