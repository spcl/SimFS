//
// Created by Pirmin Schmid on 01.05.17.
//

#ifndef DV_CACHES_FILECACHES_FILECACHEPARTITIONAWAREBASE_H_
#define DV_CACHES_FILECACHES_FILECACHEPARTITIONAWAREBASE_H_

#include <unordered_map>
#include <unordered_set>

#include "../../DVBasicTypes.h"
#include "../../DVForwardDeclarations.h"
#include "FileCacheLRU.h"

namespace dv {

	/**
	 * Provides the partition dictionary and the additional functionality needed in
	 * put(), refresh(), and evict() in the partition aware caches.
	 *
	 * It is basically an extension of LRU with partition-awareness.
	 * It is an abstract class. Use PLRU, PBCL, PDCL or PACL as working implementations
	 *
	 * penalty_factor must be in range [0.0, 1.0] with 1.0 == no penalty; 0.0 == penalty to cost 0
	 * Uses the same modification re cost: uses actual_cost that returns cost if file
	 * was accessed by client, or 0 otherwise
	 */
	class FileCachePartitionAwareBase : public FileCacheLRU {
	public:

		FileCachePartitionAwareBase(DV *dv_ptr, ID_type capacity, double penalty_factor, bool use_unit_cost);

		virtual void put(const std::string &key, std::unique_ptr<FileDescriptor> value) override;

		virtual void refresh(const std::string &key) override;


	protected:

		virtual id_cost_pair_type evict() override;

		double penalty_factor_;
		bool use_unit_cost_;

		std::unordered_map<dv::id_type, std::unordered_set<std::string>> partitions_;


	private:
		static constexpr char kCacheName[] = "PartitionAwareBase: ";
		static constexpr bool kCostAware = false;
		static constexpr bool kPartitionAware = true;


	};

}


#endif //DV_CACHES_FILECACHES_FILECACHEPARTITIONAWAREBASE_H_
