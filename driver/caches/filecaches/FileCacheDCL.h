//
// Created by Pirmin Schmid on 18.04.17.
//

#ifndef DV_CACHES_FILECACHES_FILECACHEDCL_H_
#define DV_CACHES_FILECACHES_FILECACHEDCL_H_

#include <string>

#include "FileCacheBCL.h"

namespace dv {

	class FileCacheDCL : public FileCacheBCL {
	public:
		FileCacheDCL(DV *dv_ptr, ID_type capacity, ID_type protected_mrus);

		// no change in public API
		// do not forget to use initializeWithFiles() to get the initial files into the cache

	protected:
		virtual void actualPut(const std::string &key, std::unique_ptr<FileDescriptor> value) override;

		virtual FileDescriptor *actualGet(const std::string &key) override;

		virtual id_cost_pair_type findVictim() override;

		virtual bool isCostAware() const override;

		virtual bool isPartitionAware() const override;

		void etdPut(std::string evicted_key, dv::cost_type cost);

		typedef toolbox::LinkedMap<std::string, dv::cost_type> etd_map_type;
		typedef etd_map_type::ID_type etd_ID_type;
		static constexpr etd_ID_type kEtdNone = etd_map_type::kNone;

		etd_map_type etd_;
		etd_ID_type etd_capacity_;

		dv::cost_type a_cost_ = 0;
		std::string lru_key_ = "";

	private:
		static constexpr char kCacheName[] = "DCL cache: ";
		static constexpr bool kCostAware = true;
		static constexpr bool kPartitionAware = false;
	};

}

#endif //DV_CACHES_FILECACHES_FILECACHEDCL_H_
