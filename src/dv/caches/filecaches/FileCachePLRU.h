//
// Created by Pirmin Schmid on 15.07.17.
//

#ifndef DV_CACHES_FILECACHES_FILECACHEPLRU_H
#define DV_CACHES_FILECACHES_FILECACHEPLRU_H

#include "FileCachePartitionAwareBase.h"

namespace dv {

	/**
	 * partition aware version of LRU.
	 * uses a unit cost for files, which is reduced to 0 for all files of a partition if a file of it is evicted.
	 * Files with cost 0 are then evicted before any other file.
	 */
	class FileCachePLRU : public FileCachePartitionAwareBase {
	public:
		FileCachePLRU(DV *dv_ptr, ID_type capacity);

		// no change in public API
		// do not forget to use initializeWithFiles() to get the initial files into the cache

	protected:
		virtual void actualPut(const std::string &key, std::unique_ptr<FileDescriptor> value) override;

		virtual FileDescriptor *actualGet(const std::string &key) override;

		virtual id_cost_pair_type findVictim() override;

		virtual bool isCostAware() const override;

		virtual bool isPartitionAware() const override;

		id_cost_pair_type findVictim_PLRU();

	private:
		static constexpr char kCacheName[] = "PLRU cache: ";
		static constexpr bool kCostAware = false;
		static constexpr bool kPartitionAware = true;

	};

}

#endif //DV_CACHES_FILECACHES_FILECACHEPLRU_H
