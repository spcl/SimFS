/*------------------------------------------------------------------------------
 * CppToolbox: Version
 * https://github.com/pirminschmid/CppToolbox
 *
 * Copyright (c) 2017, Pirmin Schmid, MIT license
 *----------------------------------------------------------------------------*/

#include "Version.h"

namespace toolbox {

void Version::print(std::ostream *out) {
    *out << name_ << " v" << major_ << "." << minor_ << "." << patch_ << " (" << date_ << ") " << std::endl;
}

}
