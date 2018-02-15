//
// Created by Pirmin Schmid on 15.04.17.
//


#include <iostream>
#include "DVStats.h"

namespace dv {

	void DVStats::incTotal() {
		++total_;
	}

	dv::counter_type DVStats::getTotal() const {
		return total_;
	}

	void DVStats::incHits() {
		++hits_;
	}

	dv::counter_type DVStats::getHits() const {
		return hits_;
	}

	void DVStats::incMisses() {
		++misses_;
	}

	dv::counter_type DVStats::getMisses() const {
		return misses_;
	}

	void DVStats::incWaiting() {
		++waiting_;
	}

	dv::counter_type DVStats::getWaiting() const {
		return waiting_;
	}

	void DVStats::incEvictions(std::string fileName) {
        std::cout << "log_evict " << fileName << std::endl;
		++evictions_;
	}

	dv::counter_type DVStats::getEvictions() const {
		return evictions_;
	}

	void DVStats::incFifoQueueEvictions(std::string fileName) {
		std::cout << "log_fifo_queue_evict " << fileName << std::endl;
		++fifo_queue_evictions_;
	}

	dv::counter_type DVStats::getFifoQueueEvictions() const {
		return fifo_queue_evictions_;
	}

	void DVStats::incResim(dv::counter_type amount) {
		total_resim_ += amount;
	}

	dv::counter_type DVStats::getResim() const {
		return total_resim_;
	}

	void DVStats::print(std::ostream *out) const {
		*out << "access stats: "
			 << total_ << " total, "
			 << hits_ << " hits, "
			 << misses_ << " misses, "
			 << waiting_ << " waiting, "
			 << evictions_ << " evictions, "
			 << fifo_queue_evictions_ << " FIFO queue evictions, "
			 << total_resim_ << " total re-simulations"
			 << std::endl;
	}

}
