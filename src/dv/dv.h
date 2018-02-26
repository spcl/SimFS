//
// 05/2017: Pirmin Schmid
//

// helper include file that holds some constants
// that are needed for getopt to compile, too.

#define PACKAGE "dv"
#define VERSION "v0.9.17"

#include "server/DV.h"
#include <string>
#include "getopt/dv_cmdline.h"


static const char kName[] = "SimFS DV server";
static constexpr int kVersionMajor = 0;
static constexpr int kVersionMinor = 9;
static constexpr int kVersionPatch = 17;
static const char kReleaseDate[] = "2017-09-26";

dv::DV * DVCreate(gengetopt_args_info &args_info, const std::string &config_file);

