/*------------------------------------------------------------------------------
 * CppToolbox: StatisticsHelper
 * https://github.com/pirminschmid/CppToolbox
 *
 * Copyright (c) 2017, Pirmin Schmid, MIT license
 *----------------------------------------------------------------------------*/

#include "StatisticsHelper.h"

#include <cmath>
#include <algorithm>


namespace toolbox {

double StatisticsHelper::sum(const std::vector<double> &values) {
    const auto n = values.size();
    if (n == 0) {
        return 0.0;
    }

    std::vector<double> sorted_values(values);
    std::sort(sorted_values.begin(), sorted_values.end());
    return calcSum(sorted_values);
}

double StatisticsHelper::median(const std::vector<double> &values) {
    const auto n = values.size();
    if (n == 0) {
        return kNaN;
    }

    std::vector<double> sorted_values(values);
    std::sort(sorted_values.begin(), sorted_values.end());

    const auto m = n / 2;

    if (n % 2 == 0) {
        return 0.5 * (sorted_values[m - 1] + sorted_values[m]);
    } else {
        return sorted_values[m];
    }
}

double StatisticsHelper::arithmeticMean(const std::vector<double> &values) {
    const auto n = values.size();
    if (n == 0) {
        return kNaN;
    }

    std::vector<double> sorted_values(values);
    std::sort(sorted_values.begin(), sorted_values.end());

    const MeanAndCI result = calcArithmeticMeanAndCI(sorted_values, true);
    return result.mean;
}

double StatisticsHelper::harmonicMean(const std::vector<double> &values) {
    const auto n = values.size();
    if (n == 0) {
        return kNaN;
    }

    std::vector<double> sorted_values(values);
    std::sort(sorted_values.begin(), sorted_values.end());

    const MeanAndCI result = calcHarmonicMeanAndCI(sorted_values, true);
    return result.mean;
}

double StatisticsHelper::geometricMean(const std::vector<double> &values) {
    const auto n = values.size();
    if (n == 0) {
        return kNaN;
    }

    std::vector<double> sorted_values(values);
    std::sort(sorted_values.begin(), sorted_values.end());

    const MeanAndCI result = calcGeometricMeanAndCI(sorted_values, true);
    return result.mean;
}

const StatisticsHelper::Summary StatisticsHelper::calcSummary(const std::vector<double> &values, ContentType content) {
    Summary result = {};

    result.available_data = content;

    const count_type n = values.size();
    if (n == 0) {
        result = { content, 0,
                   kNaN, kNaN, kNaN, kNaN, kNaN,
                   kNaN, kNaN, kNaN, kNaN, kNaN,
                   kNaN, kNaN, kNaN,
                   kNaN, kNaN, kNaN
                 };
        return result;
    }

    result.count = n;

    std::vector<double> sorted_values(values);
    std::sort(sorted_values.begin(), sorted_values.end());

    if ((kRobust & content) == kRobust) {
        if (1 < n) {
            result.min = sorted_values.front();
            result.max = sorted_values.back();

            result.median = getPercentile(sorted_values, 0.5);

            if (2 < n) {
                // just the bare minimum to work
                // of course these quartiles will have large 95% confidence intervals by themselves
                // thus: choose larger n for meaningful results, of course
                result.q1 = getPercentile(sorted_values, 0.25);
                result.q3 = getPercentile(sorted_values, 0.75);
            } else {
                result.q1 = result.min;
                result.q3 = result.max;
            }
        } else {
            const double value = values[0];
            result.median = value;
            result.min = value;
            result.q1 = value;
            result.q3 = value;
            result.max = value;
        }
    }

    if ((kParametric & content) == kParametric) {
        // assuming normal distribution
        MeanAndCI meanAndCI = calcArithmeticMeanAndCI(sorted_values, false);
        result.mean = meanAndCI.mean;
        result.sd = meanAndCI.sd;
        result.sem = meanAndCI.sem;
        result.ci95_a = meanAndCI.ci95_a;
        result.ci95_b = meanAndCI.ci95_b;
    }

    if ((kHarmonicMean & content) == kHarmonicMean) {
        MeanAndCI meanAndCI = calcHarmonicMeanAndCI(sorted_values, false);
        result.harmonic_mean = meanAndCI.mean;
        result.harmonic_mean_ci95_a = meanAndCI.ci95_a;
        result.harmonic_mean_ci95_b = meanAndCI.ci95_b;
    }

    if ((kGeometricMean & content) == kGeometricMean) {
        MeanAndCI meanAndCI = calcGeometricMeanAndCI(sorted_values, false);
        result.geometric_mean = meanAndCI.mean;
        result.geometric_mean_ci95_a = meanAndCI.ci95_a;
        result.geometric_mean_ci95_b = meanAndCI.ci95_b;
    }

    return result;
}

void StatisticsHelper::printSummary(std::ostream *out, const std::string &title,
                                    const Summary &stat, StatisticsHelper::ContentType style) {

    *out << std::endl << title << std::endl;

    bool printRobust = (kRobust & style & stat.available_data) == kRobust;
    bool printParametric = (kParametric & style & stat.available_data) == kParametric;
    bool printHarmonicMean = (kHarmonicMean & style & stat.available_data) == kHarmonicMean;
    bool printGeometricMean = (kGeometricMean & style & stat.available_data) == kGeometricMean;

    switch (stat.count) {
    case 0:
        *out << "n = 0" << std::endl;
        return;

    case 1:
        *out << "value =  " << stat.mean
             << ", n = " << stat.count << std::endl;
        return;

    case 2:
    case 3:
        if (printRobust) {
            *out << "- robust:       "
                 << "median " << stat.median
                 << ", min " << stat.min
                 << ", max " << stat.max
                 << ", n = " << stat.count << std::endl;
        }
        break;

    default:
        if (printRobust) {
            *out << "- robust:       "
                 << "median " << stat.median
                 << ", IQR [" << stat.q1 << ", " << stat.q3
                 << "], min " << stat.min
                 << ", max " << stat.max
                 << ", n=" << stat.count << std::endl;
        }

    }

    if (printParametric) {
        *out << "- normal dist.: "
             << "mean " << stat.mean
             << ", SD " << stat.sd
             << ", 95% CI for the mean [" << stat.ci95_a << ", " << stat.ci95_b
             << "], n = " << stat.count << std::endl;
    }

    if (printHarmonicMean) {
        *out << "- harmonic mean: " << stat.harmonic_mean
             << ", 95% CI for the mean [" << stat.harmonic_mean_ci95_a << ", " << stat.harmonic_mean_ci95_b
             << "], n = " << stat.count << std::endl;
    }

    if (printGeometricMean) {
        *out << "- geometric mean: " << stat.geometric_mean
             << ", 95% CI for the mean [" << stat.geometric_mean_ci95_a << ", " << stat.geometric_mean_ci95_b
             << "], n = " << stat.count << std::endl;
    }
}

void StatisticsHelper::calcAndPrintSummary(std::ostream *out, const std::string &title,
        const std::vector<double> &values,
        StatisticsHelper::ContentType style) {

    printSummary(out, title, calcSummary(values, style), style);
}


//--- private helper functions ---------------------------------------------

double StatisticsHelper::calcSum(const std::vector<double> &sorted_values) {
    double sum = 0.0;
    double c = 0.0;

    for (const double v : sorted_values) {
        double y = v - c;
        double t = sum + y;
        c = (t - sum) - y;
        sum = t;
    }

    return sum;
}

const StatisticsHelper::MeanAndCI StatisticsHelper::calcArithmeticMeanAndCI(const std::vector<double> &sorted_values, bool only_mean) {
    MeanAndCI result = {};
    const auto n = sorted_values.size();

    if (n == 0) {
        return result;
    }

    const double mean = calcSum(sorted_values) / static_cast<double>(n);
    result.mean = mean;

    if (only_mean) {
        return result;
    }

    if (n == 1) {
        result.sd = 0.0;
        result.sem = 0.0;
        result.ci95_a = result.mean;
        result.ci95_b = result.mean;
        return result;
    }

    std::vector<double> deltas(n);
    count_type count = 0;
    for (const double v : sorted_values) {
        const double delta = v - mean;
        deltas[count++] = delta * delta;
    }

    // use sum() here, and not calcSum(), to sort the deltas first
    const double variance_estimate = sum(deltas) / static_cast<double>(n - 1);
    const double sd = std::sqrt(variance_estimate);
    result.sd = sd;
    const double sem = sd / std::sqrt(static_cast<double>(n));
    result.sem = sem;

    const double ci95_delta = getTValue(n - 1) * sem;
    result.ci95_a = mean - ci95_delta;
    result.ci95_b = mean + ci95_delta;

    return result;
}

const StatisticsHelper::MeanAndCI StatisticsHelper::calcHarmonicMeanAndCI(const std::vector<double> &sorted_values, bool only_mean) {
    MeanAndCI result = {};
    const auto n = sorted_values.size();

    if (n == 0) {
        return result;
    }

    std::vector<double> inverses;
    inverses.reserve(n);

    for (const double v : sorted_values) {
        if (v > 0.0) {
            inverses.push_back(1.0 / v);
        } else {
            result = { kNaN, kNaN, kNaN, kNaN, kNaN };
            return result;
        }
    }

    const MeanAndCI am = calcArithmeticMeanAndCI(inverses, only_mean);

    if (0.0 < am.mean) {
        result.mean = 1.0 / am.mean;

        if (only_mean) {
            return result;
        }

        if (0.0 < am.ci95_b) {
            result.ci95_a = 1.0 / am.ci95_b;
        }

        if (0.0 < am.ci95_a) {
            result.ci95_b = 1.0 / am.ci95_a;
        } else {
            result.ci95_b = kPosInf;
        }
    } else {
        result = { kNaN, kNaN, kNaN, kNaN, kNaN };
        return result;
    }

    return result;
}

const StatisticsHelper::MeanAndCI StatisticsHelper::calcGeometricMeanAndCI(const std::vector<double> &sorted_values, bool only_mean) {
    MeanAndCI result = {};
    const auto n = sorted_values.size();

    if (n == 0) {
        return result;
    }

    std::vector<double> logs;
    logs.reserve(n);

    for (const double v : sorted_values) {
        if (v > 0.0) {
            logs.push_back(std::log(v));
        } else {
            result = { kNaN, kNaN, kNaN, kNaN, kNaN };
            return result;
        }
    }

    const MeanAndCI am = calcArithmeticMeanAndCI(logs, only_mean);
    result.mean = std::exp(am.mean);

    if (only_mean) {
        return result;
    }

    result.ci95_a = std::exp(am.ci95_a);
    result.ci95_b = std::exp(am.ci95_b);

    return result;
}

double StatisticsHelper::getPercentile(const std::vector<double> &sorted_values, double percentile) {
    const double n = static_cast<double>(sorted_values.size());

    const double r_p = 0.5 + percentile * n;
    const double r_floor = std::floor(r_p);
    const double r_frac = r_p - r_floor; // in range [0, 1), also == abs(f_floor - r_p)

    if (r_frac < kPercentileAbsoluteTolerance) {
        // integer number: use (r_floor - 1) as array index and return value
        const count_type r_index = static_cast<count_type>(r_floor) - 1;
        return sorted_values[r_index];
    } else {
        // use linear interpolation for fractional part
        // benefits and limitations -> see mentioned paper
        const count_type r_index = static_cast<count_type>(r_floor) - 1;
        const count_type r_index2 = r_index + 1;
        double result = (1.0 - r_frac) * sorted_values[r_index]; // the closer to 0.0, the more weight
        result += r_frac * sorted_values[r_index2];              // the closer to 1.0, the more weight
        return result;
    }
}

double StatisticsHelper::getTValue(count_type df) {
    if (df < 1) {
        return -1.0;
    }

    for (const auto &item : t_table_) {
        if (df >= item.df) {
            return item.t_value;
        }
    }

    // will not happen
    return -1.0;
}


const std::vector<StatisticsHelper::Distr> StatisticsHelper::t_table_ = {
    {StatisticsHelper::kTableInfinity, 1.960},
    {500, 1.965},
    {300, 1.968},
    {200, 1.9719},
    {150, 1.9759},
    {120, 1.9799},
    {100, 1.9840},
    {90, 1.9867},
    {80, 1.9901},
    {70, 1.9944},
    {60, 2.0003},
    {50, 2.0086},
    {48, 2.0106},
    {46, 2.0129},
    {44, 2.0154},
    {42, 2.0181},
    {40, 2.0211},
    {39, 2.0227},
    {38, 2.0244},
    {37, 2.0262},
    {36, 2.0281},
    {35, 2.0301},
    {34, 2.0322},
    {33, 2.0345},
    {32, 2.0369},
    {31, 2.0395},
    {30, 2.0423},
    {29, 2.0452},
    {28, 2.0484},
    {27, 2.0518},
    {26, 2.0555},
    {25, 2.0595},
    {24, 2.0639},
    {23, 2.0687},
    {22, 2.0739},
    {21, 2.0796},
    {20, 2.0860},
    {19, 2.0930},
    {18, 2.1009},
    {17, 2.1098},
    {16, 2.1199},
    {15, 2.1315},
    {14, 2.1448},
    {13, 2.1604},
    {12, 2.1788},
    {11, 2.2010},
    {10, 2.2281},
    {9, 2.2622},
    {8, 2.3060},
    {7, 2.3646},
    {6, 2.4469},
    {5, 2.5706},
    {4, 2.7764},
    {3, 3.1824},
    {2, 4.3027},
    {1, 12.7062}
};

}
