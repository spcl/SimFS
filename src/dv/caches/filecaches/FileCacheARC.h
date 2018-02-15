//
// Created by Pirmin Schmid on 21.09.17.
//

#ifndef DV_CACHES_FILECACHES_FILECACHEARC_H_
#define DV_CACHES_FILECACHES_FILECACHEARC_H_

#include "../../DVForwardDeclarations.h"
#include "FileCache.h"
#include "FileDescriptor.h"
#include "../../server/DVStats.h"
#include "../../toolbox/LinkedMap.h"

namespace dv {

	/**
	 * Adaptive Replacement Cache (ARC) cache system
	 *
	 * implementation of following description
	 * Nimrod Megiddo and Dharmendra S. Modha. ARC: A SELF-TUNING, LOW OVERHEAD REPLACEMENT CACHE
	 * Proceedings of FAST ’03: 2nd USENIX Conference on File and Storage Technologies
	 * San Francisco, CA, USA March 31–April 2, 2003
	 *
	 * see mainly Fig. 4 of this paper.
	 *
	 * note: the mentioned fetch() operation is highly asynchronous in the setting of DV,
	 * i.e. for missing files, either a simulation is restarted or the client has to wait
	 * until an already running simulation is producing the file.
	 *
	 * Due to technical reasons (actual FileDescriptor object needed; name of detected miss
	 * is not enough) and to remain symmetric with the other cache implementations, this
	 * asynchronous fetch() is implemented as adding/keeping these file descriptors in the
	 * separate waiting list until simulator has generated the file, and then execute the
	 * actual replacement algorithm as described in Fig. 4.
	 *
	 * Actually, all moves from one part of the cache to another will only happen in the put()
	 * and refresh() methods, but not in get(). Based on defined use of all the caches here,
	 * all get() uses in the cache client code actually has a put() or refresh() afterwards.
	 *
	 * This policy additionally keeps the cache consistent (i.e. avoids having potentially
	 * lots of file descriptors in T1 or T2 that are not actually referring to physically
	 * existing files) and also keeps the views of cache hits and cache misses consistent
	 * between cache view and client view. Other considered implementations of this
	 * highly asynchronous fetch() here would lead to different views in cache and client,
	 * which is not desirable.
	 *
	 * Nevertheless, such changes can influence the performance of the ARC cache if
	 * compared with published values. This may differ more for some traces than others.
	 * Needs evaluation.
	 *
	 * Finally, simulators create files in bulk. These files may be desired/requested
	 * or not based on client trajectories. These files have to be introduced into the
	 * cache, internally leading to potentially lots of cache flushes.
	 * See also the FIFO wrapper in combination with ARC as a potential solution to this
	 * problem.
	 *
	 * technical notes:
	 * - ARC(c): cache size = size of T1 and T2
	 * - DBL(2c): 2 * cache size = size of T1, T2, B1, and B2
	 * - T1 basically refers to recency; T2 additionally refers to frequency
	 * - note: B1 and B2 hold only file descriptors without actual files -> evicted by this cache
	 *   They also do not have client descriptors or sockets to notify. Thus, no copy/merging of
	 *   internal data from the old descriptor. However, to be compatible
	 *   with the other cache implementations that return ptrs for existing files and files
	 *   being (scheduled to be) simulated, internally, this distinction must be made, too.
	 *   But for proper cache management, freshly simulated files cannot just be added to the
	 *   waiting map. Thus, the following solution:
	 *   - B1 and B2 hold descriptors of evicted files as defined. nullptr is returned for these
	 *     files to clients.
	 *   - put() additionally checks whether a file may already have a descriptor in B1 or B2
	 *     if yes, it removes it for incoming descriptors without file available and adds the new
	 *     descriptor to waiting list but with map flags where it came from (B1 or B2).
	 *     This allows proper cache handling when the refresh is being handled. It also protects
	 *     these descriptors from being removed from B1/B2. Of course, if put() is called with a
	 *     descriptor of an existing file, case handling is performed directly.
	 *     Ptrs to the file descriptor are returned for descriptors in waitingB1 and waitingB2
	 *     as usually for all descriptors in the waiting list.
	 *   - this solution allows for proper cache internal behavior and also return values
	 *     as expected from the other cache types.
	 *
	 * technical modifications of the algorithm:
	 * - fetch() operation is highly asynchronous -> see waiting list; updates only during put()
	 *   and refresh() but not get(). -> potential performance differences
	 * - bulk production of lots of files by simulators -> different performance characteristics possible
	 * - only files can be evicted that are not locked by client or simulator. Thus, modification of
	 *   the algorithm to find eviction victim (similar to all caches here, i.e. LRU that fulfills
	 *   the predicate). This may also influence performance of the cache.
	 *   -> this needs to subtle case handlings in put() and refresh() to avoid double entries
	 *      and proper case management of the described cache cases
	 *
	 * v1.0 2017-09-21 / 2017-09-26 ps
	 */
	class FileCacheARC : public FileCache {
	public:
		typedef toolbox::LinkedMap<std::string, std::unique_ptr<FileDescriptor>> cache_map_type;
		typedef cache_map_type::ID_type ID_type;

		static constexpr ID_type kMaxCapacity = std::numeric_limits<ID_type>::max();
		static constexpr ID_type kNone = -1;

		FileCacheARC(DV *dv_ptr, ID_type capacity);

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


	private:
		struct waiting_descriptor {
			std::unique_ptr<FileDescriptor> fd;
			int type; // 0 regular, 1 B1, 2 B2
		};

		typedef std::unordered_map<std::string, waiting_descriptor> waiting_map_type;

		enum map_type {kT1, kT2, kB1, kB2, kWaitingRegular, kWaitingB1, kWaitingB2, kNotFound};

		struct location_type {
			map_type map;
			ID_type id; // for T1, T2, B1, B2
			std::unique_ptr<FileDescriptor> *w_u_ptr; // for waiting list (regular, B1, and B2)
		};

		DV *dv_ptr_;

		cache_map_type T1_;
		cache_map_type T2_;
		cache_map_type B1_;
		cache_map_type B2_;

		ID_type total_B1_size();
		ID_type total_B2_size();

		waiting_map_type waiting_;
		ID_type waiting_count(int type);

		ID_type capacity_;
		bool debug_messages_;
		std::string cache_name_;

		dv::id_type p_ = 0; // target T1 size; (capacity - p) is target T2 size

		static constexpr char kCacheName[] = "ARC cache: ";

		std::vector<std::string> access_trace_;

		toolbox::KeyValueStore statusSummary_;

		location_type find(const std::string &key);

		/**
		 * replace function as defined in Fig. 4 of the paper.
		 * p is read/adjusted directly (see p_)
		 */
		void replace(const location_type &xt);

		/**
		 * file must be evictable (i.e. no lock by clients and simulators)
		 */
		void evictFile(FileDescriptor *fd);

		/**
		 * finds LRU based evictable filedescriptor in the given map
		 */
		ID_type findVictim(const cache_map_type &map);


		/**
		 * case distinctions; used by refresh() and put()
		 */
		void refresh_internal(const std::string &key, const location_type &location);

		/**
  		 * handles updates for cache entries as defined in Fig. 4
  		 * Used in refresh_internal()
  		 */
		void handleUpdate(const std::string &key, const location_type &location);

		/**
		 * handles updates for cache entries as defined in Fig. 4
		 * Used in put() and indirectly from refresh() handling waiting list
		 */
		void handleCaseIV(const std::string &key, const location_type &location, std::unique_ptr<FileDescriptor> value);

		/**
		 * helper function
		 * note: must return nullptr for B1 and B2 cases but return ptr for WaitingB1 and WaitingB2
		 */
		inline FileDescriptor *location2ptr(const location_type &location);

		/** internal helper function that also returns ptr for B1 and B2 */
		inline FileDescriptor *location2ptr_internal(const location_type &location);
	};

}

#endif //DV_CACHES_FILECACHES_FILECACHEARC_H_
