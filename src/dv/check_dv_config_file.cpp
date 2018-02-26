//
// allows checking of config files without running the entire server
//
// additionally to a Lua interpreter, it offers all callback functions
// as defined in DV.
//
// Created by Pirmin Schmid on 11.05.17.
//

#include <iostream>
#include <string>

#include "server/DVConfig.h"
#include "toolbox/Version.h"

using namespace std;
using namespace dv;
using namespace toolbox;

const string name = "Check DV config file";
const int version_major = 1;
const int version_minor = 0;
const int version_patch = 0;
const string kReleaseDate = "2017-05-11";


void error_exit(const string &name, const string &additional_text) {
    cout << endl << "Usage: " << name << " <DV config file>" << endl;
    cout << additional_text << endl;
    cout << endl;
    exit(1);
}


int main(int argc, char *argv[]) {
    cout << endl;
    Version version(name, version_major, version_minor, version_patch, kReleaseDate);
    version.print(&cout);
    cout << endl;

    string program_name = argv[0];
    if (argc != 2) {
        error_exit(program_name, "Wrong number of arguments.");
    }

    DVConfig dv_config;
    if (!dv_config.loadConfigFile(argv[1])) {
        error_exit(program_name, "Config script could not be loaded.");
    }


    if (dv_config.init() == 0) {
        error_exit(program_name, "Config script returned error code during init(). Check Lua syntax, too.");
    }

    cout << "init() OK" << endl;

    if (dv_config.run_checks() == 0) {
        error_exit(program_name, "Config script returned error code during run_checks().");
    }

    cout << "run_checks() OK" << endl;

    return 0;
}
