// 
// 04/2017: Porting/rewriting from SDG's python version (PS)
//

#include "JobQueue.h"

#include <iostream>

#include "../simulator/SimJob.h"
#include "../DVLog.h"

namespace dv {

void JobQueue::setMaxSimJobs(int32_t max_simjobs) {
    max_simjobs_ = max_simjobs;
}

void JobQueue::logState(const char * msg) {
    LOG(CLIENT, 1, "JobQueue, running: " + std::to_string(current_simjobs_) + "/" + std::to_string(max_simjobs_) + "; queue: " + std::to_string(queue_.size()) +"; " + std::string(msg));
}

void JobQueue::enqueue(SimJob *simjob_ptr) {
    if (current_simjobs_ < max_simjobs_) {
        current_simjobs_++;
        logState("DIRECT LAUNCH");
        simjob_ptr->launch();
    } else {
        logState("QUEUEING");
        queue_.push_back(simjob_ptr);
        //simjob_ptr->launch();
    }
}

void JobQueue::handleJobTermination() {
    if (0 < queue_.size()) {
        logState("ended job -> de-queueing a new job");
        queue_.front()->launch();
        queue_.pop_front();
    } else {
        current_simjobs_--;
        logState("ended job");

        if (current_simjobs_ < 0) {
            LOG(ERROR, 0, "negative number of simjobs!");
        }
    }
}

const std::deque<SimJob *> &JobQueue::queue() const {
    return queue_;
}

}
