#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include "config.h"

#include "getopt/dv_cmdline.h"
#include "server/DV.h"
#include "dv.h"
#include "SimFS.hpp"
#include "SimFSEnv.hpp"

using namespace dv;
using namespace toolbox;
using namespace std;
using namespace simfs;

volatile int gdbstop;


void error_exit(const std::string &name, const std::string &additional_text) {
    std::cout << "Usage: " << name << " <command> [OPTIONS]" << std::endl;
    std::cout << "\t" << name << " help for list of OPTIONS" << std::endl;
    std::cout << std::endl;
    LOG(ERROR, 0, additional_text);
    exit(1);
}

void initLogger() {

    /* Note: this order has to reflect the macro numbering! (see below)*/
    const char * keynames[] = {KEYNAMES, NULL};

    char * log_keys = getenv("SIMFS_LOG");
    char * log_level_str = getenv("SIMFS_LOG_LEVEL");

    int log_level = log_level_str==NULL? 0 : atoi(log_level_str);

    Logger::setLogKey(ERROR, 10, keynames[0], LOG_FULL, ANSI_COLOR_RED);
    Logger::setLogKey(INFO, -1, keynames[1], LOG_SIMPLE, ANSI_COLOR_YELLOW);
    Logger::setLogKey(SIMULATOR, -1, keynames[2], LOG_SIMPLE, ANSI_COLOR_BLUE);
    Logger::setLogKey(CLIENT, -1, keynames[3], LOG_SIMPLE, ANSI_COLOR_GREEN);
    Logger::setLogKey(CACHE, -1, keynames[4], LOG_SIMPLE, ANSI_COLOR_YELLOW);
    Logger::setLogKey(WARNING, -1, keynames[5], LOG_SIMPLE, ANSI_COLOR_YELLOW);
    Logger::setLogKey(PREFETCHER, -1, keynames[6], LOG_SIMPLE, ANSI_COLOR_MAGENTA);

    if (log_keys!=NULL) {
        std::string keys_str = std::string(log_keys);
        const char ** keyptr = keynames;
    
        while (*keyptr!=NULL) {
            if (keys_str.find(std::string(*keyptr))!=std::string::npos) {
                /* Strong assumption about jeynames, MACRO ordering/numbering! */
                Logger::setLogKeyLevel(ERROR + (keyptr-keynames), log_level);
            }
            keyptr++;
        }
    }
}



int main(int argc, char * argv[]) {

    /* Let GDB in */
    int attach_gdb = getenv("DV_ATTACH_GDB")!=NULL;
    if (attach_gdb) {
        gdbstop=1;
        std::cout << "Waiting for GDB (PID: " << getpid() << "; type 'set gdbstop=0' to continue)." << std::endl;
        while (gdbstop) {;}
    }

    /* Init logger */
    initLogger();

    /* Load SimFS */
    SimFS fs;
    if (fs.load()!=0){
        error_exit(argv[0], "Failed to load SimFS!");
    }


    /* Parsing & checking arguments */
    if (argc < 3) {
        error_exit(argv[0], "An envirorment and a command are expected!");
    }

    std::string envname(argv[1]);
    std::string cmd(argv[2]);


    /* Executing commands*/
    if (cmd == SIMFS_INIT) {

        if (argc!=4) {
            error_exit(argv[0], "Invalid syntax. See usage.");
        }

        std::string conf_file = argv[3];
        SimFSEnv env(fs, envname, conf_file);

    } else if (cmd == SIMFS_RUN || cmd == SIMFS_SIMULATE) {
        if (argc<3) {
            error_exit(argv[0], "Invalid syntax. See usage.");
        }

        /* Load env */
        SimFSEnv env(fs, envname);
        if (!env.isValid()){
            error_exit(argv[0], "Error while initializing the environment!");
        }   

        /* LD_PRELOADING */
        const char * libname = getenv("SIMFS_DVLIB");
        if (libname==NULL) libname = DEFAULT_DVLIB;
        setenv("LD_PRELOAD", (SIMFS_DVLIB + string(libname)).c_str(), 1);

        printf("SimFS envirorment: %s\n", envname.c_str());

    
        /* Check if there is a known IP/PORT for it */
        simfs_env_addr_t addr;
        if (!env.getLastKnownAddress(&addr)){

            printf("DV should be listening on %s:%s\n", addr.ip.c_str(), addr.port.c_str());

            setenv("DV_PROXY_SRV_IP", addr.ip.c_str(), 1);
            setenv("DV_PROXY_SRV_PORT", addr.port.c_str(), 1);
        }

        /* Simulator needs specific env var */
        if (cmd == SIMFS_SIMULATE){
            setenv("DV_SIMULATOR", "1", 1);
        }

        /* Run! */
        printf("Running %s!\n", argv[3]);
        execvpe(argv[3], &(argv[3]), environ);


    } else if (cmd == SIMFS_START || cmd == SIMFS_START_PASSIVE) {

        gengetopt_args_info args_info;

        //std::string env(argv[2]);
        argv[2] = argv[0];

        /* Parse DV options */
        cmdline_parser(argc-2, &(argv[2]), &args_info);
        if (cmdline_parser(argc, argv, &args_info) != 0) {
            error_exit(argv[0], "Error while parsing command line arguments");
        }

        /* Load env */
        SimFSEnv env(fs, envname);
        if (!env.isValid()){
            error_exit(argv[0], "Error while initializing the environment!");
        }   
                
        /* Create DV (server) */
        DV * dv = DVCreate(args_info, env.getConfigFile());
        if (dv==NULL) {
            error_exit(argv[0], "Failed to start the server!");
        }

        /* Save the IP/PORT on which we are listening */
        simfs_env_addr_t addr;
        addr.ip = dv->getIpAddress();
        addr.port = dv->getClientPort();
    
        env.setNewAddress(addr);
        env.save(); /* so they are immediately visible */

        KeyValueStore fileidx;
        if (cmd == SIMFS_START_PASSIVE) { 

            env.loadFiles(&fileidx);

            dv->setPassive();
            dv->setFileIndex(&fileidx);
        }
    
        printf("STARTING HERE!!!");
        dv->run();

        if (cmd == SIMFS_START_PASSIVE) { 
            env.saveFiles(&fileidx);
        }

        delete dv;

        cmdline_parser_free(&args_info);

    } else if (cmd == SIMFS_LS){


    } else if (cmd == SIMFS_INDEX){
        /*
        simfs::simfs_env_t env;
        KeyValueStore fileidx;

        if (argc!=4){
            error_exit(argv[0], "Invalid syntax. See usage.");
        }

        char * file = argv[3];

        simfs::findEnviron(&env);
        simfs::loadFileIndex(env, &fileidx);
    
        //TODO: handle the case where a directory is passed as argument
        if (!FileSystemHelper::fileExists(file)){
            LOG(ERROR, "File not found!");
            exit(1);
        }    

        fileidx.setString(
        */

    } else if (cmd == SIMFS_INFO){


    } else {
        printf("Command not recognized!\n");
        exit(1);
    }


    /* Cleaning up */

    return 0;
}

