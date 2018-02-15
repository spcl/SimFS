//
// Created by Pirmin on 01.05.17.
//

#ifndef DV_CACHES_FILECACHES_FILECACHEPBCL_H_
#define DV_CACHES_FILECACHES_FILECACHEPBCL_H_


#include "FileCachePartitionAwareBase.h"

namespace dv {

	/**
	 * partition aware version of BCL.
	 * Code is *identical* to BCL, except different constructor and different base class.
	 */
	class FileCachePBCL : public FileCachePartitionAwareBase {
	public:
		FileCachePBCL(DV *dv_ptr, ID_type capacity, ID_type protected_mrus, double penalty_factor);

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
		static constexpr char kCacheName[] = "PBCL cache: ";
		static constexpr bool kCostAware = true;
		static constexpr bool kPartitionAware = true;

	};
}

#endif //DV_CACHES_FILECACHES_FILECACHEPBCL_H_
