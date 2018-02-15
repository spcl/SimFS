//
// Created by Pirmin Schmid on 01.05.17.
//

#include "FileCachePartitionAwareBase.h"

#include <iostream>

#include "../../server/DV.h"
#include "../../simulator/Simulator.h"
#include "../../toolbox/FileSystemHelper.h"


namespace dv {

	constexpr char FileCachePartitionAwareBase::kCacheName[];

	FileCachePartitionAwareBase::FileCachePartitionAwareBase(DV *dv_ptr,
															 FileCacheLRU::ID_type capacity,
															 double penalty_factor,
															 bool use_unit_cost) :
			FileCacheLRU(dv_ptr, capacity), penalty_factor_(penalty_factor), use_unit_cost_(use_unit_cost) {
		cache_name_ = kCacheName;
	}

	//--- public interface -----------------------------------------------------
	// basically identical to LRU except small modifications for partition handling


	void FileCachePartitionAwareBase::put(const std::string &key, std::unique_ptr<FileDescriptor> value) {
		if (debug_messages_) {
			std::cout << cache_name_ << "put_before: " << std::endl;
			printStatus(&std::cout);
		}

		dv::cost_type cost = 1;
		if (use_unit_cost_) {
			value->setUnitCost();
		} else {
			cost = dv_ptr_->getSimulatorPtr()->getCost(value->getName());
			value->setCost(cost);
		}

		dv::id_type partition_key = dv_ptr_->getSimulatorPtr()->partitionKey(key);
		value->setPartitionKey(partition_key);

		dv::id_type id_nr = dv_ptr_->getSimulatorPtr()->result2nr(value->getName());
		value->setNr(id_nr);

		// protect against double insertions (should not happen if code is correct)
		ID_type cache_id = cache_.find(key);
		auto waiting_it = waiting_.find(key);
		if (!(cache_id == kNone && waiting_it == waiting_.end())) {
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
			// handle partition dictionary
			auto it = partitions_.find(partition_key);
			if (it == partitions_.end()) {
				partitions_[partition_key] = {key};
			} else {
				it->second.emplace(key);
			}

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
			if (use_unit_cost_) {
				std::cout << " has fixed unit cost, but 0 if part of a partition with holes." << std::endl;
			} else {
				std::cout << " has cost " << cost << " if accessed by clients, reduced otherwise, "
						  << "and reduced by penalty_factor if part of a partition with holes." << std::endl;
			}
			printStatus(&std::cout);
		}
	}

	void FileCachePartitionAwareBase::refresh(const std::string &key) {
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

						// set partition key
						dv::id_type partition_key = dv_ptr_->getSimulatorPtr()->partitionKey(key);
						w->setPartitionKey(partition_key);

						// handle partition dictionary
						auto it = partitions_.find(partition_key);
						if (it == partitions_.end()) {
							partitions_[partition_key] = {key};
						} else {
							it->second.emplace(key);
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


	//--- internal protected interface -----------------------------------------
	// only modification in evict (mainly identical to LRU with small modification

	FileCacheLRU::id_cost_pair_type FileCachePartitionAwareBase::evict() {
		id_cost_pair_type r = findVictim();
		if (r.first == kNone) {
			std::cerr << cache_name_
					  << "could not evict a file from cache. Too many files are open/locked at the moment for given cache capacity "
					  << capacity_ << ". Capacity will be increased to continue with service. Start service with larger capacity."
					  << std::endl;
			r.second = kCostForNone;
			return r;
		}

		// get descriptor
		FileDescriptor *descriptor = cache_.get(r.first)->get();
		dv_ptr_->getStatsPtr()->incEvictions(descriptor->getName());


		// apply penalty to all files of the partition
		dv::id_type partition_key = descriptor->getPartitionKey();
		dv::id_type id_nr = descriptor->getNr();
		auto it = partitions_.find(partition_key);
		if (it == partitions_.end()) {
			std::cerr << "FileCachePartitionAwareBase::evict(): ERROR: partition key not found." << std::endl;
		} else {
			// apply & collect keys to be removed
			std::vector<std::string> keys_to_remove;
			for (const std::string &k : it->second) {
				ID_type d = cache_.find(k);
				if (d != kNone) {
					FileDescriptor *desc = cache_.get(d)->get();
					if (desc->getNr() <= id_nr) {
						desc->applyPenaltyFactor(penalty_factor_);
						keys_to_remove.push_back(k);
					}
				}
			}

			// remove processed keys
			for (const std::string &k : keys_to_remove) {
				it->second.erase(k);
			}
		}

		// remove file
		if (toolbox::FileSystemHelper::fileExists(descriptor->getFileName())) {
			toolbox::FileSystemHelper::rmFile(descriptor->getFileName());
			if (debug_messages_) {
				std::cout << cache_name_ << "removed file " << descriptor->getFileName() << std::endl;
			}
		} else {
			std::cerr << cache_name_ << "evict(): file " << descriptor->getFileName() << " should be removed, but does not exist." << std::endl;
			// continue, file is already "not there"
		}

		// eviction by replacement happens for the cache database in actualPut()
		return r;
	}
}
