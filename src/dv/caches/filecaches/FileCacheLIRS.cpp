//
// Created by Pirmin Schmid on 15.07.17.
//

#include <cassert>
#include "FileCacheLIRS.h"

#include "../../server/DV.h"
#include "../../toolbox/FileSystemHelper.h"

namespace dv {

	//--- static ---------------------------------------------------------------

	constexpr char FileCacheLIRS::kCacheName[];

	bool FileCacheLIRS::isStateWithFile(FileCacheLIRS::State state) {
		switch (state) {
			case kLIR:
			case kResidentHIR:
			case kResidentHIRnotInSqueue:
				return true;
			default:
				return false;
		}
	}

	//--- MetaData -------------------------------------------------------------

	FileCacheLIRS::MetaData::MetaData(std::unique_ptr<FileDescriptor> fileDescriptor,
									  FileCacheLIRS::State state,
									  FileCacheLIRS::clock_type now,
									  DVStats *stats)
			: fileDescriptor_(std::move(fileDescriptor)), state_(state),
			  current_(now), stats_(stats) {}

	FileCacheLIRS::MetaData::MetaData(std::unique_ptr<FileDescriptor> fileDescriptor,
									  FileCacheLIRS::State state,
									  FileCacheLIRS::clock_type now,
									  const FileCacheLIRS::MetaData &history,
									  DVStats *stats) {
		current_ = history.current_;
		irr_ = history.irr_;
		stats_ = stats;
		setStateWithFileDescriptor(state, std::move(fileDescriptor), now);
	}

	bool FileCacheLIRS::MetaData::isFileAvailable() const {
		return fileDescriptor_.get() != nullptr;
	}

	FileDescriptor *FileCacheLIRS::MetaData::getFileDescriptor() const {
		return fileDescriptor_.get();
	}

	void FileCacheLIRS::MetaData::setState(FileCacheLIRS::State state, FileCacheLIRS::clock_type now) {
		setCurrent(now);
		setStateWithoutClockChange(state);
	}

	void FileCacheLIRS::MetaData::setStateWithFileDescriptor(FileCacheLIRS::State state,
															 std::unique_ptr<FileDescriptor> fileDescriptor,
															 FileCacheLIRS::clock_type now) {
		assert(isStateWithFile(state));
		fileDescriptor_ = std::move(fileDescriptor);
		setState(state, now);
	}

	void FileCacheLIRS::MetaData::setStateWithoutClockChange(FileCacheLIRS::State state) {
		state_ = state;

		// adjust file descriptor (which will also remove a file if needed)
		if (!isStateWithFile(state_)) {
			FileDescriptor *fd = fileDescriptor_.get();
			if (fd != nullptr) {
				// remove file
				if (toolbox::FileSystemHelper::fileExists(fd->getFileName())) {
					toolbox::FileSystemHelper::rmFile(fd->getFileName());
					std::cout << "Cache: removed file " << fd->getFileName() << std::endl;
				} else {
					std::cerr << "Cache: file " << fd->getFileName()
							  << " should be removed, but does not exist."
							  << std::endl;
					std::cout << "Cache: file " << fd->getFileName()
							  << " should be removed, but does not exist."
							  << std::endl;
					// continue, file is already "not there"
				}

				// remove filedescriptor
				fileDescriptor_.release();

				// adjust stats
				stats_->incEvictions(fd->getName());
			}
		}

		// assertions
		if (isStateWithFile(state_)) {
			assert(fileDescriptor_.get() != nullptr);
		} else {
			assert(fileDescriptor_.get() == nullptr);
		}
	}

	FileCacheLIRS::State FileCacheLIRS::MetaData::getState() const {
		return state_;
	}

	void FileCacheLIRS::MetaData::setCurrent(FileCacheLIRS::clock_type now) {
		irr_ = now - current_;
		current_ = now;
	}

	FileCacheLIRS::clock_type FileCacheLIRS::MetaData::getRecency(FileCacheLIRS::clock_type now) {
		return now - current_;
	}

	FileCacheLIRS::clock_type FileCacheLIRS::MetaData::getIRR() {
		return irr_;
	}


	//--- LIRS: public API -----------------------------------------------------

	FileCacheLIRS::FileCacheLIRS(DV *dv_ptr,
								 FileCacheLIRS::ID_type total_capacity,
								 FileCacheLIRS::ID_type lir_capacity)
			: FileCache(), dv_ptr_(dv_ptr), dv_stats_(dv_ptr->getStatsPtr()), cache_name_(kCacheName),
			  total_capacity_(total_capacity),
			  lir_capacity_(lir_capacity),
			  resident_hir_capacity_(total_capacity - lir_capacity),
			  s_queue_(total_capacity < (kMaxCapacity >> 1) ? 2 * total_capacity : total_capacity),
			  q_queue_(total_capacity - lir_capacity) {

		// note: S queue is initialized with larger internal capacity than actual capacity
		// since it will hold various meta data without actual files (see non-resident HIR files,
		// but also additional waiting list and deletable info)
		assert(lir_capacity < total_capacity);
		assert(0 < total_capacity && 0 < lir_capacity);

		debug_messages_ = dv_ptr_->getConfigPtr()->filecache_debug_output_on_;
		s_queue_.setDebugMode(dv_ptr_->getConfigPtr()->filecache_details_debug_output_on_);
		q_queue_.setDebugMode(dv_ptr_->getConfigPtr()->filecache_details_debug_output_on_);
	}

	void FileCacheLIRS::initializeWithFiles() {
		std::cout << cache_name_ << "initialized with capacity " << total_capacity_
				  << " and Q queue capacity " << resident_hir_capacity_ << std::endl;

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

		std::cout  << cache_name_ << q_queue_.size() << " files cached." << std::endl;
	}

	void FileCacheLIRS::put(const std::string &key, std::unique_ptr<FileDescriptor> value) {
		if (debug_messages_) {
			std::cout << cache_name_ << "put_before: " << std::endl;
			printStatus(&std::cout);
		}

		dv::cost_type cost = dv_ptr_->getSimulatorPtr()->getCost(value->getName());
		value->setCost(cost);

		// protect against double insertions (should not happen if code is correct)
		bool error = false;
		ID_type cache_id = s_queue_.find(key);
		if (cache_id != kNone) {
			State s = s_queue_.get(cache_id)->get()->getState();
			if (isStateWithFile(s)) {
				error = true;
			}

			// it is OK for use if it refers to a state that is not associated with a file
			// state adjustment will be made
		}

		auto waiting_it = waiting_.find(key);
		if (waiting_it != waiting_.end()) {
			error = true;
		}

		if (error) {
			std::cerr << "ERROR in " << cache_name_ << "key already exists. Check usage of put() in client code."
					  << std::endl;
			std::cerr << "NO change was made to the cache database!" << std::endl;

			std::cout << "ERROR in " << cache_name_ << "key already exists. Check usage of put() in client code."
					  << std::endl;
			std::cout << "NO change was made to the cache database!" << std::endl;

			// any change would only introduces consecutive additional errors.
			// the client code MUST be changed to proper use of the cache in this case
			return;
		}

		// regular case
		if (value->isFileAvailable()) {
			actualPut(key, std::move(value), cache_id);
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
				std::cout << " has cost " << cost << " if accessed by clients, reduced otherwise" << std::endl;
			} else {
				std::cout << " has fixed unit cost " << std::endl;
			}
			printStatus(&std::cout);
		}

#ifdef DV_CACHES_FILECACHES_FILECACHELIRS_RUN_INVARIANT_ASSURANCE_TESTS
		assureInvariants("after put " + key);
#endif
	}

	/**
	 * note: the status update happens during the refresh() that is called in the client
	 * code after each call of get() with successful retrieval of a file descriptor.
	 */
	FileDescriptor *FileCacheLIRS::get(const std::string &key) {
#ifdef DV_CACHES_FILECACHES_FILECACHELIRS_RUN_INVARIANT_ASSURANCE_TESTS
		assureInvariants("before get " + key);
#endif

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

	FileDescriptor *FileCacheLIRS::internal_lookup_get(const std::string &key) {
#ifdef DV_CACHES_FILECACHES_FILECACHELIRS_RUN_INVARIANT_ASSURANCE_TESTS
		assureInvariants("before internal get " + key);
#endif

		ID_type id = s_queue_.find(key);
		if (id == kNone) {
			auto it = waiting_.find(key);
			if (it == waiting_.end()) {
				return nullptr;
			}
			return it->second.get();
		}

		MetaData *mdp = s_queue_.get(id)->get();
		if (!isStateWithFile(mdp->getState())) {
			auto it = waiting_.find(key);
			if (it == waiting_.end()) {
				return nullptr;
			}
			return it->second.get();
		}

		return mdp->getFileDescriptor();
	}

	/**
	 * refresh has to handle lots of various cases.
	 */
	void FileCacheLIRS::refresh(const std::string &key) {
		if (debug_messages_) {
			std::cout << cache_name_ << "refresh_before: " << std::endl;
			printStatus(&std::cout);
		}

		MetaData *c_mdp = nullptr;
		FileDescriptor *c = nullptr;
		ID_type c_id = s_queue_.find(key);
		if (c_id != kNone) {
			c_mdp = s_queue_.get(c_id)->get();
			if (isStateWithFile(c_mdp->getState())) {
				c = c_mdp->getFileDescriptor();
			}
		}

		FileDescriptor *w = nullptr;
		auto w_it = waiting_.find(key);
		if (w_it != waiting_.end()) {
			w = w_it->second.get();
		}

		// cache system
		// case distinction for all theoretically possible cases
		// however, only 3 typical case should actually happen
		// 1) descriptor was in cache only (as an existing file)
		//   -> just gets a refresh (move to MRU)
		// 2) descriptor was in waiting list and fileAvailable() still false
		//   -> nothing
		// 3) descriptor is in waiting list and fileAvailable() is true
		//   -> move to cache using actualPut() including potential eviction
		//      of former content
		// In all other cases, in particular if found in both or no locations
		// -> some form of error caused by wrong use of this cache library.

		// LIRS status
		// adjustment as described in the paper

		if (c != nullptr) {
			if (w == nullptr) {
				if (c->isFileAvailable()) {
					// case 1) -> simple refresh
					actualRefresh(key, c_id);
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

						ID_type cache_id = s_queue_.find(key);
						actualPut(key, std::move(waiting_[key]), cache_id);
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

#ifdef DV_CACHES_FILECACHES_FILECACHELIRS_RUN_INVARIANT_ASSURANCE_TESTS
		assureInvariants("after refresh " + key);
#endif
	}

	const std::vector<std::string> &FileCacheLIRS::getAccessTrace() const {
		return access_trace_;
	}

	const std::string &FileCacheLIRS::name() const {
		return cache_name_;
	}

	dv::id_type FileCacheLIRS::capacity() const {
		return total_capacity_;
	}

	dv::id_type FileCacheLIRS::size() const {
		Stats stats = getStats();
		return stats.count_all;
	}

	FileCollection::Stats FileCacheLIRS::getStats() const {
		// note: this predicate assumes a filtered set of inputs (only metadata with files are given as input)
		auto evictablePredicate = [](const std::unique_ptr<MetaData> &md) -> bool {
			FileDescriptor *fd = md->getFileDescriptor();
			return fd->getLockCount() == 0 && !fd->isFileUsedBySimulator();
		};

		cache_S_queue_type::ID_type max_index = s_queue_.size() - 1;
		cache_S_queue_type::map_type<Stats> mapFunction =
				[&](const std::unique_ptr<MetaData> &md, cache_S_queue_type::ID_type index) -> Stats {
					if (!isStateWithFile(md->getState())) {
						Stats result = {0, 0, 0, 0};
						return result;
					}

					FileDescriptor *fd = md->getFileDescriptor();
					const auto size = fd->getSize();
					const bool evictable = evictablePredicate(md) && index < max_index;
					Stats result = {
							1,
							evictable ? 1 : 0,
							size,
							evictable ? size : 0
					};
					return result;
				};

		cache_S_queue_type::reduce_type<Stats, Stats> reduceFunction =
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
		return s_queue_.mapreduce(zero, reduceFunction, mapFunction);
	}

	void FileCacheLIRS::printStatus(std::ostream *out) {
		const Stats stats = getStats();
		*out << cache_name_ << " capacity " << total_capacity_
			 << ", size " << stats.count_all << ", evictable " << stats.count_evictable
			 << " (" << (stats.count_all > 0 ? ((stats.count_evictable * 100) / stats.count_all) : 0)
			 << "%), waiting list size " << waiting_.size() << std::endl
			 << "file size all " << stats.filesize_all
			 << " bytes, evictable " << stats.filesize_evictable << " bytes"
			 << ", " << (stats.filesize_all > 0 ? ((stats.filesize_evictable * 100) / stats.filesize_all) : 0)
			 << "%" << std::endl;

		*out << std::endl;
	}

	const toolbox::KeyValueStore &FileCacheLIRS::getStatusSummary() {
		// update summary
		const Stats stats = getStats();
		statusSummary_.setString("cache_name", cache_name_);
		statusSummary_.setInt("cache_capacity", total_capacity_);
		statusSummary_.setInt("cache_capacity_LIR", lir_capacity_);
		statusSummary_.setInt("cache_capacity_resident_HIR", resident_hir_capacity_);
		statusSummary_.setInt("cache_size", stats.count_all);
		statusSummary_.setInt("cache_size_evictable", stats.count_evictable);
		statusSummary_.setInt("cache_size_evictable_percent", stats.count_all > 0 ? ((stats.count_evictable * 100) / stats.count_all) : 0);
		statusSummary_.setInt("cache_waiting", waiting_.size());
		statusSummary_.setInt("cache_filesize_all", stats.filesize_all);
		statusSummary_.setInt("cache_filesize_evictable", stats.filesize_evictable);
		statusSummary_.setInt("cache_filesize_evictable_percent", stats.filesize_all > 0 ? ((stats.filesize_evictable * 100) / stats.filesize_all) : 0);
		return statusSummary_;
	}


	//--- internal protected API

	void FileCacheLIRS::actualPut(const std::string &key, std::unique_ptr<FileDescriptor> value, ID_type id) {
		// based on initial checks, we can assume here that the id does not refer to a record with a file
		// this limits the cases to be handled
		// see more complex refresh method

		assert(key == value->getName());

		clock_type now = tick();

		// try using a pool space if kNone
		if (id == kNone) {
			auto poolPredicate = [](const std::unique_ptr<MetaData> &md) -> bool {
				return md->getState() == kPool;
			};

			id = s_queue_.findFirstWithPredicate(poolPredicate, false, 0);
		} else {
			assert(!isStateWithFile(s_queue_.get(id)->get()->getState()));
		}

		// case distinctions

		if (id == kNone) {
			// no former meta information available
			if (lir_size_ < lir_capacity_) {
				std::unique_ptr<MetaData> md = std::make_unique<MetaData>(std::move(value), kLIR, now, dv_stats_);
				lir_size_++;
				s_queue_.add(key, std::move(md));
			} else {
				std::unique_ptr<MetaData> md = std::make_unique<MetaData>(std::move(value), kResidentHIR, now, dv_stats_);
				if (q_queue_.size() < resident_hir_capacity_) {
					q_queue_.add(key, md.get());
					s_queue_.add(key, std::move(md));
				} else {
					ID_type q_lru = findLastResidentHIR_in_Q(true);
					if (q_lru == kNone) {
						std::cout << "Could not find evictable file in resident HIR set. Increase HIR set size." << std::endl;
						std::cerr << "Could not find evictable file in resident HIR set. Increase HIR set size." << std::endl;
						exit(1);
						// TODO: or implement silent extension of the set size
					}

					MetaData *old_mdp = *(q_queue_.get(q_lru));
					old_mdp->setStateWithoutClockChange(kNonResidentHIR); // this will also delete the file
					q_queue_.replace(q_lru, key, md.get());
					s_queue_.add(key, std::move(md));
				}
			}

		} else {
			MetaData *mdp = s_queue_.get(id)->get();
			State s = mdp->getState();
			assert(s == kPool || s == kNonResidentHIR);

			if (lir_size_ < lir_capacity_) {
				assert(s == kPool);
				// note: this path will never happen; just here to be complete (and symmetric to case distinction above)
				std::unique_ptr<MetaData> md = std::make_unique<MetaData>(std::move(value), kLIR, now, dv_stats_);
				s_queue_.replace(id, key, std::move(md));
			} else {

				if (s == kNonResidentHIR) {
					// adjust LRU of LIR set as new MRU of resident HIR
					// and make the reactivated non-resident HIR a LIR
					std::unique_ptr<MetaData> md = std::make_unique<MetaData>(std::move(value), kLIR, now, *mdp, dv_stats_);
					if (q_queue_.size() < resident_hir_capacity_) {
						ID_type s_lru = findLastLIR_in_S(false);
						if (s_lru == kNone) {
							// should not be a problem since no check for locks
							std::cout << "Could not find file in LIR set. Increase LIR set size." << std::endl;
							std::cerr << "Could not find file in LIR set. Increase LIR set size." << std::endl;
							exit(1);
						}

						MetaData *old_lir = s_queue_.get(s_lru)->get();
						old_lir->setStateWithoutClockChange(kResidentHIR);
						q_queue_.add(old_lir->getFileDescriptor()->getName(), old_lir);
						s_queue_.replace(id, key, std::move(md));

						prune_S(old_lir->getFileDescriptor()->getName());

					} else {
						// additionally remove the LRU of the resident HIR to make room for the new LIR
						ID_type q_lru = findLastResidentHIR_in_Q(true);
						if (q_lru == kNone) {
							std::cout << "Could not find evictable file in resident HIR set. Increase HIR set size." << std::endl;
							std::cerr << "Could not find evictable file in resident HIR set. Increase HIR set size." << std::endl;
							exit(1);
							// TODO: or implement silent extension of the set size
						}

						ID_type s_lru = findLastLIR_in_S(false);
						if (s_lru == kNone) {
							// should not be a problem since no check for locks
							std::cout << "Could not find file in LIR set. Increase LIR set size." << std::endl;
							std::cerr << "Could not find file in LIR set. Increase LIR set size." << std::endl;
							exit(1);
						}

						MetaData *old_hir = *(q_queue_.get(q_lru));
						old_hir->setStateWithoutClockChange(kNonResidentHIR); // this will also delete the file

						MetaData *old_lir = s_queue_.get(s_lru)->get();
						old_lir->setState(kResidentHIR, now);

						q_queue_.replace(q_lru, old_lir->getFileDescriptor()->getName(), old_lir);
						s_queue_.replace(id, key, std::move(md));

						prune_S(old_lir->getFileDescriptor()->getName());
					}

				} else {
					// kPool: like kNone just with re-using of list space
					std::unique_ptr<MetaData> md = std::make_unique<MetaData>(std::move(value), kResidentHIR, now, dv_stats_);
					if (q_queue_.size() < resident_hir_capacity_) {
						q_queue_.add(key, md.get());
						s_queue_.replace(id, key, std::move(md));
					} else {
						ID_type q_lru = findLastResidentHIR_in_Q(true);
						if (q_lru == kNone) {
							std::cout << "Could not find evictable file in resident HIR set. Increase HIR set size." << std::endl;
							std::cerr << "Could not find evictable file in resident HIR set. Increase HIR set size." << std::endl;
							exit(1);
							// TODO: or implement silent extension of the set size
						}

						MetaData *old_mdp = *(q_queue_.get(q_lru));
						old_mdp->setState(kNonResidentHIR, now); // this will also delete the file
						q_queue_.replace(q_lru, key, md.get());
						s_queue_.replace(id, key, std::move(md));
					}
				}

			}
		}

	}

	void FileCacheLIRS::actualRefresh(const std::string &key, FileCacheLIRS::ID_type id) {
		// based on initial checks, we can assume here that the id must refer to a record with a file
		// this limits the cases to be handled
		assert(id != kNone);

		clock_type now = tick();

		// case distinctions
		MetaData *mdp = s_queue_.get(id)->get();
		State s = mdp->getState();
		std::string name = mdp->getFileDescriptor()->getName();

		assert(isStateWithFile(s));
		assert(key == name);

		if (s == kLIR) {
			// move to MRU position
			s_queue_.refreshWithId(id);

			// pruning needed in case it was at LRU position
			prune_S(name);

		} else if (s == kResidentHIR) {
			ID_type q_pos = q_queue_.find(key);
			assert(q_pos != kNone);

			// 1) change LRU positioned LIR to resident HIR including adding to Q queue
			// 2) make id block to LIR and move to MRU position in S, and remove it from Q queue
			ID_type s_lru = findLastLIR_in_S(false);
			if (s_lru == kNone) {
				// should not be a problem since no check for locks
				std::cout << "Could not find file in LIR set. Increase LIR set size." << std::endl;
				std::cerr << "Could not find file in LIR set. Increase LIR set size." << std::endl;
				exit(1);
			}

			MetaData *old_lir = s_queue_.get(s_lru)->get();
			old_lir->setStateWithoutClockChange(kResidentHIR);
			q_queue_.replace(q_pos, old_lir->getFileDescriptor()->getName(), old_lir);

			mdp->setState(kLIR, now);
			s_queue_.refreshWithId(id);
			prune_S(old_lir->getFileDescriptor()->getName());

		} else if (s == kResidentHIRnotInSqueue) {
			// re-introduce it as residentHIR in S queue
			// make it MRU in Q and in S queue
			ID_type q_pos = q_queue_.find(key);
			assert(q_pos != kNone);
			mdp->setState(kResidentHIR, now);
			q_queue_.refreshWithId(q_pos);
			s_queue_.refreshWithId(id);

		} else {
			// should not happen
			std::cerr << "FileCacheLIRS::actualRefresh() error: not yet implemented case " << s << std::endl;
			exit(1);
		}
	}

	FileDescriptor *FileCacheLIRS::actualGet(const std::string &key) {
		ID_type id = s_queue_.find(key);
		if (id == kNone) {
			return nullptr;
		}

		MetaData *mdp = s_queue_.get(id)->get();
		if (!isStateWithFile(mdp->getState())) {
			return nullptr;
		}

		return mdp->getFileDescriptor();
	}


	bool FileCacheLIRS::isCostAware() const {
		return false;
	}

	bool FileCacheLIRS::isPartitionAware() const {
		return false;
	}


	//--- private helper methods -----------------------------------------------

	FileCacheLIRS::clock_type FileCacheLIRS::tick() {
		return ++clock;
	}

	void FileCacheLIRS::prune_S(const std::string &just_moved_lir_key) {
		bool pruning_active = true;
		cache_S_queue_type::map_type<FileCacheLIRS::ID_type> mapFunction =
				[&](const std::unique_ptr<MetaData> &md, cache_S_queue_type::ID_type index) -> ID_type {
					if (!pruning_active) {
						return 0;
					}

					switch (md->getState()) {
						case kLIR:
							pruning_active = false;
							return 0;

						case kResidentHIR:
							if (md->getFileDescriptor()->getName() == just_moved_lir_key) {
								return 0;
							}

							md->setStateWithoutClockChange(kResidentHIRnotInSqueue);
							// note: this will keep the resident HIR in the Q queue as defined
							return 1;

						case kNonResidentHIR:
							md->setStateWithoutClockChange(kPool);
							// note: kPool can also be seen as non-resident HIR not in S queue
							// semantically equivalent to not known to the cache
							// pool is used here to avoid unnecessary memory allocations/deallocations
							// in the actual S queue map.
							return 1;

						default:
							return 0;
					}
				};

		cache_S_queue_type::reduce_type<FileCacheLIRS::ID_type, FileCacheLIRS::ID_type> reduceFunction =
				[](const ID_type &accumulator, const ID_type &value) -> ID_type {
					return accumulator + value;
				};

		ID_type count = s_queue_.mapreduce(0, reduceFunction, mapFunction);
		if (debug_messages_) {
			std::cout << cache_name_ << count << " pruned HIR elements from S queue" << std::endl;
		}
	}

	FileCacheLIRS::ID_type FileCacheLIRS::findLastLIR_in_S(bool only_unlocked_files) {
		auto lirPredicate = [](const std::unique_ptr<MetaData> &md) -> bool {
			return md->getState() == kLIR;
		};

		auto lirAndUnlockedPredicate = [](const std::unique_ptr<MetaData> &md) -> bool {
			if (md->getState() != kLIR) {
				return false;
			}

			FileDescriptor *fd = md->getFileDescriptor();
			return fd->getLockCount() == 0 && !fd->isFileUsedBySimulator();
		};


		ID_type victim = only_unlocked_files ?
						 s_queue_.findFirstWithPredicate(lirAndUnlockedPredicate, false, 0)
											 : s_queue_.findFirstWithPredicate(lirPredicate, false, 0);
		return victim;
	}

	FileCacheLIRS::ID_type FileCacheLIRS::findLastResidentHIR_in_Q(bool only_unlocked_files) {
		if (0 == q_queue_.size()) {
			return kNone;
		}

		if (!only_unlocked_files) {
			return q_queue_.getLruId();
		}

		auto rhirAndUnlockedPredicate = [](const MetaData *mdp) -> bool {
			FileDescriptor *fd = mdp->getFileDescriptor();
			return fd->getLockCount() == 0 && !fd->isFileUsedBySimulator();
		};

		return q_queue_.findFirstWithPredicate(rhirAndUnlockedPredicate, false, 0);
	}

#ifdef DV_CACHES_FILECACHES_FILECACHELIRS_RUN_INVARIANT_ASSURANCE_TESTS
	void FileCacheLIRS::assureInvariants(const std::string &title) {
		auto sQueuePredicate = [&](const std::unique_ptr<MetaData> &md) -> bool {
			switch (md->getState()) {
				case kLIR:
					if (q_queue_.find(md->getFileDescriptor()->getName()) != kNone) {
						std::cerr << title << " INV error in S queue: LIR file found in Q queue: "
								  << md->getFileDescriptor()->getName()
								  << std::endl;
					}

					if (waiting_.find(md->getFileDescriptor()->getName()) != waiting_.end()) {
						std::cerr << title << " INV error in S queue: LIR file found in waiting list: "
								  << md->getFileDescriptor()->getName()
								  << std::endl;
					}
					break;

				case kResidentHIR:
					if (q_queue_.find(md->getFileDescriptor()->getName()) == kNone) {
						std::cerr << title << " INV error in S queue: resident HIR file NOT found in Q queue: "
								  << md->getFileDescriptor()->getName()
								  << std::endl;
					}

					if (waiting_.find(md->getFileDescriptor()->getName()) != waiting_.end()) {
						std::cerr << title << " INV error in S queue: resident HIR file found in waiting list: "
								  << md->getFileDescriptor()->getName()
								  << std::endl;
					}
					break;

				case kResidentHIRnotInSqueue:
					if (q_queue_.find(md->getFileDescriptor()->getName()) == kNone) {
						std::cerr << title << " INV error in S queue: resident HIR (not in S) file NOT found in Q queue: "
								  << md->getFileDescriptor()->getName()
								  << std::endl;
					}

					if (waiting_.find(md->getFileDescriptor()->getName()) != waiting_.end()) {
						std::cerr << title << " INV error in S queue: resident HIR (not in S) file found in waiting list: "
								  << md->getFileDescriptor()->getName()
								  << std::endl;
					}
					break;

				case kNonResidentHIR:
					if (md->getFileDescriptor() != nullptr) {
						std::cerr << title << " INV error in S queue: non-resident HIR with filedescriptor != nullptr "
								  << std::endl;
					}
					break;

				case kPool:
					if (md->getFileDescriptor() != nullptr) {
						std::cerr << title << " INV error in S queue: pool with filedescriptor != nullptr "
								  << std::endl;
					}
					break;
			}
			return false;
		};

		auto qQueuePredicate = [&](const MetaData *md) -> bool {
			ID_type lookup;
			switch (md->getState()) {
				case kResidentHIR:
					lookup = s_queue_.find(md->getFileDescriptor()->getName());
					if (lookup == kNone) {
						std::cerr << title << " INV error in Q queue: resident HIR file NOT found in S queue: "
								  << md->getFileDescriptor()->getName()
								  << std::endl;
					}

					if (waiting_.find(md->getFileDescriptor()->getName()) != waiting_.end()) {
						std::cerr << title << " INV error in Q queue: resident HIR file found in waiting list: "
								  << md->getFileDescriptor()->getName()
								  << std::endl;
					}
					break;

				case kResidentHIRnotInSqueue:
					lookup = s_queue_.find(md->getFileDescriptor()->getName());
					if (lookup == kNone) {
						std::cerr << title << " INV error in Q queue: resident HIR (not in S) file NOT found with special flag in S queue (potential null pointer exception: "
								  << md->getFileDescriptor()->getName()
								  << std::endl;
					} else {
						State s = s_queue_.get(lookup)->get()->getState();
						if (s != kResidentHIRnotInSqueue) {
							std::cerr << title << " INV error in Q queue: resident HIR (not in S) file found in S queue with wrong state: "
									  << md->getFileDescriptor()->getName()
									  << " state: " << s
									  << std::endl;
						}
					}

					if (waiting_.find(md->getFileDescriptor()->getName()) != waiting_.end()) {
						std::cerr << title << " INV error in Q queue: resident HIR (not in S) file found in waiting list: "
								  << md->getFileDescriptor()->getName()
								  << std::endl;
					}
					break;

				default:
					std::cerr << title << " INV error in Q queue: wrong state for MetaData in Q queue: " << md->getState()
							  << std::endl;
			}
			return false;
		};

		s_queue_.findFirstWithPredicate(sQueuePredicate, false, 0);
		q_queue_.findFirstWithPredicate(qQueuePredicate, false, 0);
	}
#endif

}
