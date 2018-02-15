//
// Created by Pirmin Schmid on 15.07.17.
//

#include "FileCachePLRU.h"

namespace dv {

	constexpr char FileCachePLRU::kCacheName[];

	FileCachePLRU::FileCachePLRU(DV *dv_ptr, FileCacheLRU::ID_type capacity)
			: FileCachePartitionAwareBase(dv_ptr, capacity, 0.0, true) {
		// penalty factor 0.0 sets cost to 0 for the partitions with holes
		cache_name_ = kCacheName;
	}

	void FileCachePLRU::actualPut(const std::string &key, std::unique_ptr<FileDescriptor> value) {
		FileCacheLRU::actualPut(key, std::move(value));
	}

	FileDescriptor *FileCachePLRU::actualGet(const std::string &key) {
		return FileCacheLRU::actualGet(key);
	}

	FileCacheLRU::id_cost_pair_type FileCachePLRU::findVictim() {
		id_cost_pair_type r = findVictim_PLRU();
		if (r.first != kNone) {
			return r;
		}

		// fallback
		return findVictim_LRU();
	}

	bool FileCachePLRU::isCostAware() const {
		return kCostAware;
	}

	bool FileCachePLRU::isPartitionAware() const {
		return kPartitionAware;
	}

	FileCacheLRU::id_cost_pair_type FileCachePLRU::findVictim_PLRU() {
		// run: find victims with cost == 0
		//      note: will also evict the MRU if it is the only one with cost == 0
		auto plruPredicate = [](const std::unique_ptr<FileDescriptor> &fd) {
			if (0 < fd->getLockCount()) {
				return false;
			}
			if (fd->isFileUsedBySimulator()) {
				return false;
			}
			return fd->getActualCost() == 0;
		};
		ID_type victim = cache_.findFirstWithPredicate(plruPredicate, false, 0); // also tests the MRU
		if (victim != kNone) {
			return {victim, 0};
			// cost is 0 here by definition of the predicate
		}

		// fallback is handled in findVictim
		return {kNoneWorkaround, kCostForNone};
	}

}
