//
// Created by Pirmin Schmid on 18.04.17.
//

#ifndef DV_CACHES_FILECACHES_FILECACHEBCL_H_
#define DV_CACHES_FILECACHES_FILECACHEBCL_H_


#include "FileCacheLRU.h"


namespace dv {

	class FileCacheBCL : public FileCacheLRU {
	public:
		FileCacheBCL(DV *dv_ptr, ID_type capacity, ID_type protected_mrus);

		// no change in public API
		// do not forget to use initializeWithFiles() to get the initial files into the cache

	protected:
		virtual void actualPut(const std::string &key, std::unique_ptr<FileDescriptor> value) override;

		virtual FileDescriptor *actualGet(const std::string &key) override;

		virtual id_cost_pair_type findVictim() override;

		virtual bool isCostAware() const override;

		virtual bool isPartitionAware() const override;

		id_cost_pair_type findVictim_BCL_raw();

		void resetAcost();

		void depreciateAcost(dv::cost_type amount);

		dv::cost_type a_cost_ = 0;
		std::string lru_key_ = "";

	private:
		static constexpr char kCacheName[] = "BCL cache: ";
		static constexpr bool kCostAware = true;
		static constexpr bool kPartitionAware = false;
	};

}

#endif //DV_CACHES_FILECACHES_FILECACHEBCL_H_
