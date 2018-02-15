//
// Created by Pirmin Schmid on 14.04.17.
//

#include "JobQueue.h"

#include <iostream>

#include "../simulator/SimJob.h"

namespace dv {

	void JobQueue::setMaxSimJobs(int32_t max_simjobs) {
		max_simjobs_ = max_simjobs;
	}

	void JobQueue::enqueue(SimJob *simjob_ptr) {
		if (current_simjobs_ < max_simjobs_) {
			current_simjobs_++;
			simjob_ptr->launch();
			std::cout << "[JobQueue, running " << current_simjobs_ << "/" << max_simjobs_
					  << ", queue " << queue_.size() << "]: "
					  << "direct launch." << std::endl;
		} else {
			queue_.push_back(simjob_ptr);
			std::cout << "[JobQueue, running " << current_simjobs_ << "/" << max_simjobs_
					  << ", queue " << queue_.size() << "]: "
					  << "queued job." << std::endl;
		}
	}

	void JobQueue::handleJobTermination() {
		if (0 < queue_.size()) {
			queue_.front()->launch();
			queue_.pop_front();
			std::cout << "[JobQueue, running " << current_simjobs_ << "/" << max_simjobs_
					  << ", queue " << queue_.size() << "]: "
					  << "ended job -> de-queuing new job." << std::endl;
		} else {
			current_simjobs_--;
			std::cout << "[JobQueue, running " << current_simjobs_ << "/" << max_simjobs_
					  << ", queue " << queue_.size() << "]: "
					  << "ended job." << std::endl;

			if (current_simjobs_ < 0) {
				std::cout << "[JobQueue]: ERROR: negative number of current_simjobs" << std::endl;
			}
		}
	}

	const std::deque<SimJob *> &JobQueue::queue() const {
		return queue_;
	}

}
