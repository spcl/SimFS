//
// Created by Pirmin Schmid on 14.04.17.
//

#ifndef DV_SERVER_PROFILER_H_
#define DV_SERVER_PROFILER_H_


#include <string>
#include <vector>

#include "../DVForwardDeclarations.h"
#include "../toolbox/TimeHelper.h"

namespace dv {

	class Profiler {
	public:
		Profiler();

		void reset();

		void addAlpha(double alpha);
		double getAlpha() const;
        double getMedianAlpha() const;

		double newTau();
		void extendTaus(const std::vector<double> &taus);
		void clearTaus();
		double getTau() const;
        double getMedianTau() const;

		std::string rstring() const;

		std::string toString() const;


	private:
        const double weight_ = 0.5; /* moving average weight */
		toolbox::TimeHelper::time_point_type last_time_;

		std::vector<double> alphas_;
		std::vector<double> taus_;

        double moving_alpha_=0;
        double moving_tau_=0;
	};

}

#endif //DV_SERVER_PROFILER_H_
