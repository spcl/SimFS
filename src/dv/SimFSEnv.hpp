#ifndef _SIMFSENV_HPP_
#define _SIMFSENV_HPP_

#include "toolbox/KeyValueStore.h"
#include "SimFS.hpp"

using namespace std;

typedef struct simfs_env_addr{
    string ip;
    string port;
} simfs_env_addr_t;

namespace simfs{

    class SimFSEnv{
    public:
        SimFSEnv(SimFS &fs, string name);
        SimFSEnv(SimFS &fs, string name, string conf_file);
        SimFSEnv(SimFS &fs);
        int createEnv(string name);        

        int getLastKnownAddresses(simfs_env_addr_t * addr);      
        void setNewAddresses(simfs_env_addr_t addr);
        int getLastKnownWorkingAddress(simfs_env_addr_t * addr);      
        void setNewWorkingAddress(simfs_env_addr_t addr);



        int save();

        int loadFiles(KeyValueStore * files);
        int saveFiles(KeyValueStore * files);

        string getPath();
        string getConfigFile();
            
        bool isValid();

    

    private:
        SimFS fs_;
        KeyValueStore kv_;
        string path_;
        bool is_valid_=true;
           
    private:
        int init(string env_path);
        string getKVFile();
        string getStateFile();
    };
}

#endif /* _SIMFSENV_HPP_ */
