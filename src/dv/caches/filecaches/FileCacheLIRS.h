//
// Created by Pirmin Schmid on 15.07.17.
//

#ifndef DV_CACHES_FILECACHES_FILECACHELIRS_H_
#define DV_CACHES_FILECACHES_FILECACHELIRS_H_

#include <cstdint>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../DVForwardDeclarations.h"
#include "FileCache.h"
#include "FileDescriptor.h"
#include "../../server/DVStats.h"
#include "../../toolbox/LinkedMap.h"

/**
 * A test routine can be used to test / assure that the data structure
 * invariants are true at all time while running the cache.
 * However, this option should only be run during development for
 * performance reasons. Thus, the tests can be deactivated by commenting
 * the next macro.
 *
 * runtime requirements for running the test: after put and refresh,
 * and before get / internal_get the check is run.
 * For the number of MetaData structures m in S queue (can be
 * several times larger than actual cache size n), each check runs in O(m).
 */

// #define DV_CACHES_FILECACHES_FILECACHELIRS_RUN_INVARIANT_ASSURANCE_TESTS

namespace dv {


	/**
	 * LIRS file cache
	 *
	 * implementation following the description by
	 * Song Jiang, Xiadong Zhang. LIRS: an efficient low interf-reference recency set
	 * replacement policy to improve buffer cache performance. Proceedings of the ACM
	 * SIGMETRICS Conference on Measurement and Modeling of Computer Systems (SIGMETRICS'02),
	 * Marina Del Rey, CA, June, 2002
	 *
	 * with some modifications to handle the case that locked files have to be protected
	 * from eviction.
	 *
	 * additionally some modifications re memory management: to work with unique_ptrs and
	 * avoid shared_ptrs, the following situation of the paper is redesigned.
	 * paper: resident HIR may be removed from queue S and still be part of queue Q
	 * (see case distinctions there for cases with X in S and not). This leads to the
	 * situation that the handle in Q would have to keep the ownership, which leads to
	 * shared_ptrs in both lists.
	 * -> implementation: to keep ownership only in one list, the type information in the
	 * meta-information is used to declare resident HIR without removing them from
	 * the S list: kResidentHIRnotInSqueue. And proper case distinction in the code will
	 * handle proper handling of the cases as described in the paper.
	 * Please note: this may keep the meta information object a bit longer in memory
	 * but it will be cleanly removed as soon as possible (see pruning)
	 *
	 * v1.1 2017-07-15  / 2017-08-10 ps
	 */
	class FileCacheLIRS : public FileCache {
	public:
		enum State { /* official states */ kLIR, kResidentHIR, kNonResidentHIR,
			        /* additional helpers */ kResidentHIRnotInSqueue, kPool };

		// note: kPool can also be seen as non-resident HIR not in S queue
		// as described in the paper.

		static bool isStateWithFile(State state);

		// note: LIR and resident HIR actually refer to cached files -> see cache size

		typedef dv::counter_type clock_type;

		class MetaData {
		public:
			MetaData(std::unique_ptr<FileDescriptor> fileDescriptor, State state, clock_type now, DVStats *stats);

			MetaData(std::unique_ptr<FileDescriptor> fileDescriptor, State state, clock_type now,
					 const MetaData &history, DVStats *stats);

			bool isFileAvailable() const;

			FileDescriptor *getFileDescriptor() const;

			void setState(State state, clock_type now);
			void setStateWithFileDescriptor(State state, std::unique_ptr<FileDescriptor> fileDescriptor, clock_type now);
			void setStateWithoutClockChange(State state);
			State getState() const;

			void setCurrent(clock_type now);
			clock_type getRecency(clock_type now);
			clock_type getIRR();

		private:
			std::unique_ptr<FileDescriptor> fileDescriptor_;
			State state_;
			clock_type current_;
			clock_type irr_ = std::numeric_limits<clock_type>::max();
			DVStats *stats_;

		};

		typedef toolbox::LinkedMap<std::string, std::unique_ptr<MetaData>> cache_S_queue_type;
		typedef toolbox::LinkedMap<std::string, MetaData *> cache_Q_queue_type;
		typedef cache_S_queue_type::ID_type ID_type;

		typedef std::unordered_map<std::string, std::unique_ptr<FileDescriptor>> waiting_map_type;

		static constexpr ID_type kMaxCapacity = std::numeric_limits<ID_type>::max();
		static constexpr ID_type kNone = -1;


		/**
		 * capacity is split into
		 * - LIR
		 * - HIR
		 *
		 * there are 2 queues
		 * - S queue: LIR, resident and non-resident HIR, additional helper states
		 * - Q queue: only resident HIR
		 * thus: lir_capacity + resident_hir_capacity = total_capacity
		 *
		 * S queue size typically > LIR set size
		 */
		FileCacheLIRS(DV *dv_ptr, ID_type total_capacity, ID_type lir_capacity);

		virtual void initializeWithFiles() override;

		virtual void put(const std::string &key, std::unique_ptr<FileDescriptor> value) override;

		virtual FileDescriptor *get(const std::string &key) override;

		virtual FileDescriptor *internal_lookup_get(const std::string &key) override;

		virtual void refresh(const std::string &key) override;

		virtual const std::vector<std::string> &getAccessTrace() const override;

		virtual const std::string &name() const override;

		virtual dv::id_type capacity() const override;

		virtual dv::id_type size() const override;

		virtual Stats getStats() const override;

		virtual void printStatus(std::ostream *out) override;

		virtual const toolbox::KeyValueStore &getStatusSummary() override;

	protected:
		// this is basically the internal API used by all classes inheriting from LRU
		typedef std::pair<ID_type, dv::cost_type> id_cost_pair_type;

		virtual void actualPut(const std::string &key, std::unique_ptr<FileDescriptor> value, ID_type id);

		virtual void actualRefresh(const std::string &key, ID_type id);

		virtual FileDescriptor *actualGet(const std::string &key);

		virtual bool isCostAware() const;

		virtual bool isPartitionAware() const;

		DV *dv_ptr_;
		DVStats *dv_stats_;
		bool debug_messages_;
		std::string cache_name_;

		ID_type total_capacity_;
		ID_type lir_capacity_;
		ID_type resident_hir_capacity_;
		cache_S_queue_type s_queue_;
		cache_Q_queue_type q_queue_;
		waiting_map_type waiting_;

		ID_type lir_size_ = 0;

		clock_type clock = 0;

		std::vector<std::string> access_trace_;

		// note:
		// these must be objective and const
		// and cannot be class (static) and constexpr
		const dv::cost_type kCostForLRU = -1;
		const dv::cost_type kCostForNone = -2;

		// and this is a workaround
		const ID_type kNoneWorkaround = kNone;


	private:
		static constexpr char kCacheName[] = "LIRS cache: ";
		static constexpr bool kCostAware = false;
		static constexpr bool kPartitionAware = false;

		toolbox::KeyValueStore statusSummary_;

		clock_type tick();

		/**
		 * implements the pruning operation as described in the paper.
		 * needs to be called after whenever a LIR was removed from S queue.
		 */
		void prune_S(const std::string &just_moved_lir_key);

		/**
		 * find LIR for transfer to resident HIR
		 */
		ID_type findLastLIR_in_S(bool only_unlocked_files);


		/**
		 * find resident HIR for eviction from cache
		 */
		ID_type findLastResidentHIR_in_Q(bool only_unlocked_files);

		/**
		 * helper
		 */
#ifdef DV_CACHES_FILECACHES_FILECACHELIRS_RUN_INVARIANT_ASSURANCE_TESTS
		void assureInvariants(const std::string &title);
#endif

	};

}

#endif //DV_CACHES_FILECACHES_FILECACHELIRS_H_
