//
// Created by Pirmin Schmid on 13.05.17.
//

// wrapper that helps including dv.h
// without need to re-add it to the auto-generated source code
// everytime from scratch.

#include "../dv.h"

extern "C" {

#include "dv_cmdline.c"

}
