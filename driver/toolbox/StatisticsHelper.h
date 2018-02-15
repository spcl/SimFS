/*------------------------------------------------------------------------------
 * CppToolbox: StatisticsHelper
 * https://github.com/pirminschmid/CppToolbox
 *
 * Copyright (c) 2017, Pirmin Schmid, MIT license
 *----------------------------------------------------------------------------*/

#ifndef TOOLBOX_STATISTICSHELPER_H_
#define TOOLBOX_STATISTICSHELPER_H_

#include <cstdint>

#include <limits>
#include <ostream>
#include <vector>

namespace toolbox {

	/**
	 * Some basic descriptive statistics functions.
	 *
	 * This code is based on former code published on
	 * https://github.com/pirminschmid/benchmarkC
	 * (has MIT license)
	 *
	 * References:
	 * - Lentner C (ed). Geigy Scientific Tables, 8th edition, Volume 2. Basel: Ciba-Geigy Limited, 1982
	 * - Kahan W. Further remarks on reducing truncation errors
	 *   Communications of the ACM 1965;8(1):40
	 * - Neumaier A. Rundungsfehleranalyse einiger Verfahren zur Summation endlicher Summen
	 *   ZAMM 1974;54:39-51
	 * - Press WH et al. Numerical recipes in C++ 2nd ed. Cambridge University Press
	 * - Schoonjans F. https://www.medcalc.org/manual
	 * - Schoonjans F, De Bacquer D, Schmid P. Estimation of population percentiles
	 *   Epidemiology 2011;22(5):750-1
	 *
	 * v1.2 2017-06-16 Pirmin Schmid
	 */

	class StatisticsHelper {
	public:
		typedef uint64_t count_type;

		static constexpr double kPosInf = std::numeric_limits<double>::infinity();
		static constexpr double kNegInf = -std::numeric_limits<double>::infinity();
		static constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();

		enum ContentType {
			kRobust = 1,
			kParametric = 2,
			kHarmonicMean = 4,
			kGeometricMean = 8,
			kFull = kRobust | kParametric | kHarmonicMean | kGeometricMean
		};

		struct Summary {
			ContentType available_data;

			// robust
			count_type count;
			double min;
			double q1;
			double median;
			double q3;
			double max;

			// parametric (assume normal distribution)
			double mean;   // arithmetic mean
			double sd;     // mean +/- sd
			double sem;    // standard error of the mean
			double ci95_a; // 95% confidence interval [a,b] for the mean
			double ci95_b;

			// alternative means
			double harmonic_mean;
			double harmonic_mean_ci95_a;
			double harmonic_mean_ci95_b;

			double geometric_mean;
			double geometric_mean_ci95_a;
			double geometric_mean_ci95_b;
		};

		/**
		 * uses compensated summation (by Kahan) to reduce the numerical error.
		 * additionally uses sort on the values to assure equal results
		 * for all permutations of the input vector.
		 *
		 * computation: O(n log n), memory: O(n) for vector size n
		 * error: O(1)
		 *
		 * There are even better summation algorithms, in particular for some
		 * specific corner cases. However, they typically come at cost of
		 * even more expensive computations. Thus, they are not implemented
		 * here.
		 *
		 * note: the same applies also to all other functions that base on sum(),
		 * in particular all means.
		 */
		static double sum(const std::vector<double> &values);

		/**
		 * @return median, or NaN if values.empty()
		 */
		static double median(const std::vector<double> &values);

		/**
 		 * @return arithmetic mean, or NaN if values.empty()
 		 */
		static double arithmeticMean(const std::vector<double> &values);

		/**
		 * suited for ratios and rates.
		 * harmonic mean: all values of the vector values must be > 0.0
		 *
		 * @return harmonic mean,
		 *         NaN if any of the input values <= 0.0 or values.empty()
		 */
		static double harmonicMean(const std::vector<double> &values);

		/**
		 * geometric mean: all values of the vector values must be > 0.0
		 *
		 * @return geometric mean,
		 *         NaN if any of the input values <= 0.0 or values.empty()
		 */
		static double geometricMean(const std::vector<double> &values);

		/**
		 * see comments above for specifics / restrictions of some
		 * of the returned descriptive statistic values.
		 */
		static const Summary calcSummary(const std::vector<double> &values, ContentType content = kFull);

		/**
		 * note: the output uses the floating point output settings of the provided std::ostream.
		 * Adjust precision and number output style before calling this method.
		 *
		 * only content is printed that is actually available in the summary
		 */
		static void printSummary(std::ostream *out, const std::string &title,
								 const Summary &stat, ContentType style = kFull);

		/**
		 * in case the values are not needed anymore
		 */
		static void calcAndPrintSummary(std::ostream *out, const std::string &title,
										const std::vector<double> &values, ContentType style = kFull);


	private:

		struct MeanAndCI {
			double mean;
			double sd;
			double sem;
			double ci95_a;
			double ci95_b;
		};

		static double calcSum(const std::vector<double> &sorted_values);
		static const MeanAndCI calcArithmeticMeanAndCI(const std::vector<double> &sorted_values, bool only_mean);

		/**
		 * harmonic and geometric mean:
		 * all values of the vectors must be > 0.0
		 * returns NaN for all results in MeanAndCI if any of the input values <= 0.0
		 *
		 * harmonic mean:
		 * 95% CI bounds are determined as [1/U, 1/L] for internal results [L, U] of
		 * arithmetic 95% CI calculation on inverses of the values
		 * may return +inf for ci95_b if this upper bound cannot be determined (i.e. L <= 0.0)
		 */
		static const MeanAndCI calcHarmonicMeanAndCI(const std::vector<double> &sorted_values, bool only_mean);
		static const MeanAndCI calcGeometricMeanAndCI(const std::vector<double> &sorted_values, bool only_mean);

		/**
		 *  calculates percentile value as described in https://www.medcalc.org/manual/summary_statistics.php
		 *  see Lentner C (ed). Geigy Scientific Tables, 8th edition, Volume 2. Basel: Ciba-Geigy Limited, 1982
		 *      Schoonjans F, De Bacquer D, Schmid P. Estimation of population percentiles. Epidemiology 2011;22:750-751.
		 *  percentile must be in range (0,1) here and not in (0, 100)
		 *  and 1/n <= percentile <= (n-1)/n
		 *  for n = sorted_values.size()
		 *  i.e. n must be at least 4 for percentiles 0.25 (q1) and 0.75 (q3)
		 *                          2 for percentile 0.5 (median)
		 */
		static double getPercentile(const std::vector<double> &sorted_values, double percentile);

		static constexpr double kPercentileAbsoluteTolerance = 0.001;

		/**
		 *
		 * @param df : degree of freedom
		 * @return
		 */
		static double getTValue(count_type df);

		struct Distr {
			count_type df;
			double t_value;
		};

		// values of t-distribution (two-tailed) for 100*(1-alpha)% = 95% (alpha level 0.05)
		// from Lentner C (ed). Geigy Scientific Tables, 8th edition, Volume 2. Basel: Ciba-Geigy Limited, 1982
		// and https://www.medcalc.org/manual/t-distribution.php

		static constexpr count_type kTableInfinity{1024}; // arbitrary value

		static const std::vector<Distr> t_table_;
	};

}

#endif //TOOLBOX_STATISTICSHELPER_H_
