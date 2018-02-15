//
// Created by Pirmin Schmid on 15.04.17.
//

#ifndef DV_SERVER_DVSTATS_H_
#define DV_SERVER_DVSTATS_H_

#include <ostream>

#include "../DVBasicTypes.h"

namespace dv {

	class DVStats {
	public:
		DVStats() {}

		void incTotal();
		dv::counter_type getTotal() const;

		void incHits();
		dv::counter_type getHits() const;

		void incMisses();
		dv::counter_type getMisses() const;

		void incWaiting();
		dv::counter_type getWaiting() const;

		void incEvictions(std::string fileName);
		dv::counter_type getEvictions() const;

		void incFifoQueueEvictions(std::string fileName);
		dv::counter_type getFifoQueueEvictions() const;

		void incResim(dv::counter_type amount);
		dv::counter_type getResim() const;

		void print(std::ostream *out) const;


	private:
		dv::counter_type total_ = 0;
		dv::counter_type hits_ = 0;
		dv::counter_type misses_ = 0;
		dv::counter_type waiting_ = 0;
		dv::counter_type evictions_ = 0;
		dv::counter_type fifo_queue_evictions_ = 0;
		dv::counter_type total_resim_ = 0;
	};

}

#endif //DV_SERVER_DVSTATS_H_
