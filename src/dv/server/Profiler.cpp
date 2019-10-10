//
// 01/2018: Moving AVG (SDG)
// 04/2017: Porting/rewriting from SDG's python version (PS)
//

#include "Profiler.h"

#include "DV.h"
#include "../toolbox/StatisticsHelper.h"

#define MOVING_AVG(avg, newval) (avg = avg*(1-weight_) + newval*weight_)

namespace dv {

Profiler::Profiler() {
    reset();
}

void Profiler::reset() {
    last_time_ = toolbox::TimeHelper::now();
}

void Profiler::addAlpha(double alpha) {
    last_time_ = toolbox::TimeHelper::now();
    if (alphas_.size() == 0) moving_alpha_ = alpha;
    else MOVING_AVG(moving_alpha_, alpha);
    alphas_.push_back(alpha);
    printf("PROFILER: adding alpha: %lf; new alpha: %lf (alphas_.size()=%lu)\n", alpha, moving_alpha_, alphas_.size());
}

double Profiler::getAlpha() const {
    if (alphas_.size() == 0) {
        return -1.0;
    }

    return moving_alpha_;

}

double Profiler::getMedianAlpha() const {
    if (alphas_.size() == 0 || taus_.size() == 0) {
        return -1.0;
    }

    return toolbox::StatisticsHelper::median(alphas_);
}

double Profiler::newTau() {
    toolbox::TimeHelper::time_point_type now = toolbox::TimeHelper::now();
    double newtau = toolbox::TimeHelper::seconds(last_time_, now);
    last_time_ = now;
    if (taus_.size() == 0) moving_tau_ = newtau;
    else MOVING_AVG(moving_tau_, newtau);
    taus_.push_back(newtau);
    return newtau;
}

void Profiler::extendTaus(const std::vector<double> &taus) {
    taus_.insert(taus_.end(), taus.begin(), taus.end());
    for (double tau : taus) {
        MOVING_AVG(moving_tau_, tau);
    }
}

void Profiler::clearTaus() {
    taus_.clear();
}

double Profiler::getTau() const {
    return moving_tau_;
}

double Profiler::getMedianTau() const {
    if (taus_.size() == 0) {
        return -1.0;
    }


    return toolbox::StatisticsHelper::median(taus_);
}


std::string Profiler::rstring() const {
    return std::to_string(getAlpha()) + " " + std::to_string(getTau());
}

std::string Profiler::toString() const {
    return "Alpha: " + std::to_string(getAlpha()) + "; Tau: " + std::to_string(getTau());
}

}
