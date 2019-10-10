#ifndef _SIMFS_HPP_
#define _SIMFS_HPP_


#include "toolbox/KeyValueStore.h"

#define DIR_SIMFS ".simfs"

//#define SIMFS_INSTALL_PATH "/home/salvo/ETH/SimFS/"
//#define SIMFS_WORKSPACE SIMFS_INSTALL_PATH"/simfs"
//#define SIMFS_DVLIB SIMFS_INSTALL_PATH"/lib/libdvl.so"

#define SIMFS_WSNAME "workspace"

#define CONF_NAME "conf.dv"
#define FILEIDX "index"
#define ENV_KV_NAME "env.kv"
#define ENV_STATE_NAME "env.state"


#define IPKEY(env) env + "_ip"
#define PORTKEY(env) env + "_port"

#define SIMFS_INIT "init"
#define SIMFS_START "start"
#define SIMFS_START_PASSIVE "start_passive"
#define SIMFS_LS "ls"
#define SIMFS_INDEX "index"
#define SIMFS_RUN "run"
#define SIMFS_PROFILE_RUN "profile_run"
#define SIMFS_SIMULATE "simulate"
#define SIMFS_INFO "info"
#define SIMFS_HELP "help"

#define DEFAULT_DVLIB "libdvl.so"
#define DEFAULT_PROFILE_DVLIB "libdvl_profile.so"

using namespace std;
using namespace toolbox;

namespace simfs{

    class SimFSEnv;

    class SimFS{

    public:
        SimFS();
        int load();

        string getLastEnvPath(string envname);
        void registerEnv(string envname, SimFSEnv &env);

        void save();

    private:
        KeyValueStore kv_;
    };

}

#endif /* _SIMFS_HPP_ */
