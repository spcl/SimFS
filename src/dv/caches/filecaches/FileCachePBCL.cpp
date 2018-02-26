//
// Created by Pirmin Schmid on 01.05.17.
//

#include "FileCachePBCL.h"

namespace dv {

constexpr char FileCachePBCL::kCacheName[];

FileCachePBCL::FileCachePBCL(DV *dv_ptr, ID_type capacity, ID_type protected_mrus, double penalty_factor)
    : FileCachePartitionAwareBase(dv_ptr, capacity, penalty_factor, false) {
    cache_name_ = kCacheName;
    protected_mrus_ = protected_mrus;
}

/**
 * almost identical to the LRU version, only cost handling at appropriate locations
 */
void FileCachePBCL::actualPut(const std::string &key, std::unique_ptr<FileDescriptor> value) {
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

            if (0 < r.second) {
                // adjust cost immediately
                depreciateAcost(2 * r.second);
            } else if (r.second < 0) {
                // case kCostForNone -> no eviction was possible, error message was already printed -> reset Acost
                // case kCostForLRU  -> was LRU -> reset Acost
                resetAcost();
            }
        }
    }
}

FileDescriptor *FileCachePBCL::actualGet(const std::string &key) {
    return FileCacheLRU::actualGet(key);
}

FileCachePBCL::id_cost_pair_type FileCachePBCL::findVictim() {
    id_cost_pair_type r = findVictim_BCL_raw();
    if (r.first != kNone) {
        return r;
    }

    // fallback
    return findVictim_LRU();
}

bool FileCachePBCL::isCostAware() const {
    return kCostAware;
}

bool FileCachePBCL::isPartitionAware() const {
    return kPartitionAware;
}

/**
 * note: the function here is modified in comparison to the originally described function for
 * BCL cache in Jeong and Dubois 2006. The changes are adjustments since the cache is
 * based on files and not on memory locations.
 * 1) locked files are not considered for eviction
 * 2) accordingly fallback LRU is not only considering the actual LRU but the LRU that is not locked
 *
 * additional changes (see our paper):
 * 3) a region close to MRU can be defined to be protected from BCL type eviction
 *    however, the fallback LRU will go as far as needed (but will never evict the MRU).
 * 4) actual cost is used for files, which means cost == 0 for files that were never requested by clients
 *    and cost as defined by simulator for files that were requested by clients
 * 5) BCL eviction also runs in 2 separate parts:
 *    - first scan for files with cost == 0 (including LRU; stop at MRU reserved region)
 *    - second scan using cost < Acost (exclude LRU; also stop at MRU reserved region)
 *    - and finally LRU fallback
 */
FileCachePBCL::id_cost_pair_type FileCachePBCL::findVictim_BCL_raw() {
    // run 1
    auto bclPredicate1 = [](const std::unique_ptr<FileDescriptor> &fd) {
        if (0 < fd->getLockCount()) {
            return false;
        }
        if (fd->isFileUsedBySimulator()) {
            return false;
        }
        return fd->getActualCost() == 0;
    };
    ID_type victim = cache_.findFirstWithPredicate(bclPredicate1, false, protected_mrus_);
    if (victim != kNone) {
        return {victim, 0};
        // cost is 0 here by definition of the predicate
    }

    // run 2
    dv::cost_type local_a_cost = a_cost_;
    auto bclPredicate2 = [=](const std::unique_ptr<FileDescriptor> &fd) {
        if (0 < fd->getLockCount()) {
            return false;
        }
        if (fd->isFileUsedBySimulator()) {
            return false;
        }
        return fd->getActualCost() < local_a_cost;
    };
    victim = cache_.findFirstWithPredicate(bclPredicate2, true, protected_mrus_);
    if (victim != kNone) {
        return {victim, cache_.get(victim)->get()->getActualCost()};
    }

    // fallback is handled in findVictim (note: ACL needs a BCL result without LRU fallback)
    // thus: return with none here
    return {kNoneWorkaround, kCostForNone};
}

void FileCachePBCL::resetAcost() {
    if (cache_.size() == 0) {
        a_cost_ = 0;
        lru_key_ = "";
    } else {
        FileDescriptor *lru = cache_.get(cache_.getLruId())->get();
        a_cost_ = lru->getActualCost();
        lru_key_ = lru->getName();
    }
}

void FileCachePBCL::depreciateAcost(dv::cost_type amount) {
    dv::cost_type newcost = a_cost_ - amount;
    if (newcost < 0) {
        newcost = 0;
    }
    a_cost_ = newcost;
}

}
