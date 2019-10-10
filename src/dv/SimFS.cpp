//
// 01/2018: SimFS (SDG)
//
#include "toolbox/FileSystemHelper.h"
#include "toolbox/Logger.h"
#include "toolbox/KeyValueStore.h"
#include "toolbox/NetworkHelper.h"
#include "dv.h"

#include "config.h"
#include "SimFS.hpp"

#include "SimFSEnv.hpp"


namespace simfs {

void SimFS::registerEnv(string envname, SimFSEnv &env){
    kv_.setString(envname, env.getPath());
    save();
}

SimFS::SimFS(){ ; } 

string SimFS::getLastEnvPath(string envname){
    if (!kv_.hasKey(envname)) {
        LOG(ERROR, 0, "This environment does not exist!");
        return "";
    }

    return kv_.getString(envname);
}


/* Loads the SimFS workspace. This is general, not simulation dependent */
int SimFS::load() {
    /* check if the workspace exists */
    LOG(INFO, 0, std::string("SimFS workspace: ") + SIMFS_WORKSPACE + std::string("/") + SIMFS_WSNAME);

    if (!FileSystemHelper::folderExists(SIMFS_WORKSPACE)) {
        LOG(INFO, 0, "Workspace does not exist... creating a new one!");
        if (FileSystemHelper::mkDir(SIMFS_WORKSPACE)) {
            LOG(ERROR, 0, "Error while creating the workspace!")
            return -1;
        }
    }

    /* load simfs general configuration */
    kv_.fromFile(SIMFS_WORKSPACE + std::string("/") + SIMFS_WSNAME);

    return 0;

}

void SimFS::save() {
    kv_.toFile(SIMFS_WORKSPACE + std::string("/") + SIMFS_WSNAME);
}


}






