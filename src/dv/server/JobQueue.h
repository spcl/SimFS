//
// Created by Pirmin Schmid on 14.04.17.
//

#ifndef DV_JOBS_JOBQUEUE_H_
#define DV_JOBS_JOBQUEUE_H_

#include <cstdint>
#include <deque>

#include "../DVForwardDeclarations.h"

namespace dv {

	/**
	 * note on implementation.
	 * std::queue uses a std::deque container in the back as a default.
	 * In contrast to queue, deque allows iteration over the elements.
	 * Thus, std::deque is used directly here instead of a std::queue.
	 */

	class JobQueue {
	public:
		JobQueue(int32_t max_simjobs = 0) : max_simjobs_(max_simjobs) {}

		void setMaxSimJobs(int32_t max_simjobs);

		void enqueue(SimJob *simjob_ptr);

		void handleJobTermination();

		const std::deque<SimJob *> &queue() const;

	private:
		int32_t max_simjobs_;
		int32_t current_simjobs_ = 0;
		std::deque<SimJob *> queue_;
	};

}

#endif //DV_JOBS_JOBQUEUE_H
