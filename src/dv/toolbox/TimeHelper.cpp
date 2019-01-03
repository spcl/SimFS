/*------------------------------------------------------------------------------
 * CppToolbox: TimeHelper
 * https://github.com/pirminschmid/CppToolbox
 *
 * Copyright (c) 2017, Pirmin Schmid, MIT license
 *----------------------------------------------------------------------------*/

#include "TimeHelper.h"

namespace toolbox {

TimeHelper::time_point_type TimeHelper::now() {
    return clock_type::now();
}

double TimeHelper::seconds(const TimeHelper::time_point_type &start,
                           const TimeHelper::time_point_type &end) {

    return seconds_type(end - start).count();
}

double TimeHelper::milliseconds(const TimeHelper::time_point_type &start,
                           const TimeHelper::time_point_type &end) {

    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}



}
