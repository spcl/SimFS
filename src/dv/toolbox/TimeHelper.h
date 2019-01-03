/*------------------------------------------------------------------------------
 * CppToolbox: TimeHelper
 * https://github.com/pirminschmid/CppToolbox
 *
 * Copyright (c) 2017, Pirmin Schmid, MIT license
 *----------------------------------------------------------------------------*/

#ifndef TOOLBOX_TIMEHELPER_H_
#define TOOLBOX_TIMEHELPER_H_

#include <chrono>

/**
 * Basically a convenience wrapper for chrono of the C++ standard library.
 */

namespace toolbox {

	class TimeHelper {
	public:

		typedef std::chrono::high_resolution_clock clock_type;
		typedef std::chrono::time_point<clock_type> time_point_type;
		typedef std::chrono::duration<double> seconds_type;

		static time_point_type now();

		static double seconds(const time_point_type &start, const time_point_type &end);

		static double milliseconds(const time_point_type &start, const time_point_type &end);


	};

}

#endif //TOOLBOX_TIMEHELPER_H_
