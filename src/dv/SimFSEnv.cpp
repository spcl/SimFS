#include "SimFSEnv.hpp"
#include "toolbox/FileSystemHelper.h"
#include "DVLog.h"

namespace simfs{

int SimFSEnv::init(string env_path){
        
    path_ = env_path;
    string conf_file = getConfigFile();

    if (!FileSystemHelper::fileExists(conf_file)) {
        LOG(ERROR, 0, "SimFS configuration file not found: " + conf_file);
        return -2;
    }

    LOG(INFO, 0, "Configuration file found in: " + conf_file);

    kv_.fromFile(getStateFile());

    return 0;
}


/* find a SimFS environment */
SimFSEnv::SimFSEnv(SimFS &fs): fs_(fs) {
    string current_path = FileSystemHelper::getCwd();
    string base_path = "";
    while (!FileSystemHelper::folderExists(current_path + "/" + DIR_SIMFS) && current_path!="/") {
        base_path = FileSystemHelper::getBasename(current_path) + "/" + base_path;
        current_path = FileSystemHelper::getDirname(current_path);
    }

    if (current_path=="/") {
        LOG(ERROR, 0, "This is not a SimFS envirorment!");
        is_valid_ = false;
        return;
    }

    if (init(current_path + "/" + DIR_SIMFS)!=0){
        is_valid_ = false;
        return;
    }
}


SimFSEnv::SimFSEnv(SimFS &fs, string name): fs_(fs){

    string env_path = fs_.getLastEnvPath(name);
    if (env_path=="") {
        LOG(ERROR, 0, "This environment does not exist!");
        is_valid_ = false;
        return;
    }
    if (init(env_path)!=0) { is_valid_ = false; }
}

SimFSEnv::SimFSEnv(SimFS &fs, string name, string conf_file): fs_(fs){
    /* create local folder */
    if (toolbox::FileSystemHelper::folderExists(".simfs")) {
        LOG(INFO, 0, "SimFS is already initialized here: overwriting!");
    }
    else if (toolbox::FileSystemHelper::mkDir(".simfs")) {
        LOG(ERROR, 0, "Failed to initialize SimFS!\n");
        is_valid_ = false;
        return;
    }

    /* copy conf file */
    toolbox::FileSystemHelper::cpFile(conf_file, DIR_SIMFS + std::string("/") + CONF_NAME);

    string current_path = FileSystemHelper::getCwd();
    int res = init(current_path);
    if (res!=0) { is_valid_ = false; return; }

    /* save key in the workspace */
    fs_.registerEnv(name, *this);

}



int SimFSEnv::loadFiles(KeyValueStore * files){

    if (FileSystemHelper::fileExists(getKVFile())){
        files->fromFile(getKVFile());
    }

    return 0;
}

int SimFSEnv::saveFiles(KeyValueStore * files){
    files->toFile(getKVFile());
    return 0;
}

void SimFSEnv::save(){
    kv_.toFile(getStateFile());

}

int SimFSEnv::getLastKnownAddress(simfs_env_addr_t * addr){
    if (kv_.hasKey("ip") && kv_.hasKey("port")){
        addr->ip = kv_.getString("ip");
        addr->port = kv_.getString("port");
        
        return 0;
    }
    return -1;
}

void SimFSEnv::setNewAddress(simfs_env_addr_t addr){
    kv_.setString("ip", addr.ip);
    kv_.setString("port", addr.port);
}

bool SimFSEnv::isValid(){
    return is_valid_;
}

string SimFSEnv::getPath(){
    return path_;
}

string SimFSEnv::getStateFile(){
    return path_ + std::string("/") + DIR_SIMFS + std::string("/") + ENV_STATE_NAME;
}

string SimFSEnv::getKVFile(){
    return path_ + std::string("/") + DIR_SIMFS + std::string("/") + ENV_KV_NAME;
}

string SimFSEnv::getConfigFile(){
    return path_ + std::string("/") + DIR_SIMFS + std::string("/") + CONF_NAME;
}

}
