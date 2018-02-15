//
// Created by Pirmin Schmid on 18.04.17.
//

#ifndef DV_CACHES_FILECACHES_FILECACHELRU_H_
#define DV_CACHES_FILECACHES_FILECACHELRU_H_

#include <cstdint>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../DVForwardDeclarations.h"
#include "FileCache.h"
#include "FileDescriptor.h"
#include "../../toolbox/LinkedMap.h"

namespace dv {

	/**
	 * implements an LRU cache
	 * no protected MRUs here
	 */
	class FileCacheLRU : public FileCache {
	public:
		typedef toolbox::LinkedMap<std::string, std::unique_ptr<FileDescriptor>> cache_map_type;
		typedef cache_map_type::ID_type ID_type;

		typedef std::unordered_map<std::string, std::unique_ptr<FileDescriptor>> waiting_map_type;

		static constexpr ID_type kMaxCapacity = std::numeric_limits<ID_type>::max();
		static constexpr ID_type kNone = -1;

		FileCacheLRU(DV *dv_ptr, ID_type capacity);

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

		virtual void actualPut(const std::string &key, std::unique_ptr<FileDescriptor> value);

		virtual FileDescriptor *actualGet(const std::string &key);

		virtual id_cost_pair_type evict();

		/**
		 * several id_cost_pair_types are used as signals
		 * @return
		 *
		 * pair<kNone, kCostForNone>  if no victim was found
		 * pair<id, kCostForLRU>                 id was found using LRU search
		 * pair<id, cost>                        id was found with some cost >= 0
		 */
		virtual id_cost_pair_type findVictim();

		virtual bool isCostAware() const;

		virtual bool isPartitionAware() const;

		id_cost_pair_type findVictim_LRU();

		DV *dv_ptr_;
		bool debug_messages_;
		std::string cache_name_;

		ID_type capacity_;
		ID_type protected_mrus_;
		cache_map_type cache_;
		waiting_map_type waiting_;

		std::vector<std::string> access_trace_;

		// note:
		// these must be objective and const
		// and cannot be class (static) and constexpr
		const dv::cost_type kCostForLRU = -1;
		const dv::cost_type kCostForNone = -2;

		// and this is a workaround
		const ID_type kNoneWorkaround = kNone;


	private:
		static constexpr char kCacheName[] = "LRU cache: ";
		static constexpr bool kCostAware = false;
		static constexpr bool kPartitionAware = false;

		toolbox::KeyValueStore statusSummary_;
	};

}

#endif //DV_CACHES_FILECACHES_FILECACHELRU_H_
