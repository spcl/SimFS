//
// Created by Pirmin Schmid on 01.05.17.
//

#ifndef DV_CACHES_FILECACHES_FILECACHEPDCL_H_
#define DV_CACHES_FILECACHES_FILECACHEPDCL_H_

#include "FileCachePBCL.h"

namespace dv {

	class FileCachePDCL : public FileCachePBCL {
	public:
		FileCachePDCL(DV *dv_ptr, ID_type capacity, ID_type protected_mrus, double penalty_factor);

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
		static constexpr char kCacheName[] = "PDCL cache: ";
		static constexpr bool kCostAware = true;
		static constexpr bool kPartitionAware = true;
	};

}


#endif //DV_CACHES_FILECACHES_FILECACHEPDCL_H_
