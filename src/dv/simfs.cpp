#include "toolbox/FileSystemHelper.h"
#include "toolbox/Logger.h"
#include "toolbox/KeyValueStore.h"
#include "toolbox/NetworkHelper.h"
#include "getopt/dv_cmdline.h"
#include "dv.h"

#include "config.h"


#include "server/DV.h"

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>


#define DIR_SIMFS ".simfs"

//#define SIMFS_INSTALL_PATH "/home/salvo/ETH/SimFS/"
//#define SIMFS_WORKSPACE SIMFS_INSTALL_PATH"/simfs"
//#define SIMFS_DVLIB SIMFS_INSTALL_PATH"/lib/libdvl.so"

#define SIMFS_WSNAME "workspace"

#define CONF_NAME "conf.dv"

#define IPKEY(env) env + "_ip"
#define PORTKEY(env) env + "_port"

#define SIMFS_INIT "init"
#define SIMFS_START "start"
#define SIMF_LS "ls"
#define SIMFS_INDEX "index"
#define SIMFS_RUN "run"


#define DEFAULT_DVLIB "libdvl.so"

using namespace dv;
using namespace toolbox;
using namespace std;

volatile int gdbstop;

namespace simfs { 

    /* Search a configuration file. 
     * Starts from the CWD up to the root, looking for .simfs/conf.dv. 
    */
    string findConf(){
        string current_path = FileSystemHelper::getCwd();
        while (!FileSystemHelper::folderExists(current_path + "/" + DIR_SIMFS) && current_path!="/"){
            current_path = FileSystemHelper::getDirname(current_path);
        }

        if (current_path=="/"){
            LOG(ERROR, 0, "This is not a SimFS envirorment!");
            return "";
        }
               
        string conf_file = current_path + "/" + DIR_SIMFS + "/" + CONF_NAME;
        
        if (!FileSystemHelper::fileExists(conf_file)) {
            LOG(ERROR, 0, "SimFS configuration file not found!");
            return "";
        }
        
        LOG(INFO, 0, "Configuration file found in: " + conf_file);

        return conf_file;
    }

    /* Loads the SimFS workspace. This is general, not simulation dependent */
    int loadWorkspace(KeyValueStore &ws){
        /* check if the workspace exists */
        if (!FileSystemHelper::folderExists(SIMFS_WORKSPACE)){
            if (FileSystemHelper::mkDir(SIMFS_WORKSPACE)) return 0;
        }

        /* load simfs general configuration */
        ws.fromFile(SIMFS_WORKSPACE + std::string("/") + SIMFS_WSNAME);

        return 1;

    }
    
    /* Saves the SimFS workspace */
    void saveWorkspace(KeyValueStore &ws){
        ws.toFile(SIMFS_WORKSPACE"/workspace");
    }


    /* Initializes a virtualized envirorment (i.e., simulation). */
    int initEnvirorment(KeyValueStore &ws, std::string name, std::string conf_file){

        /* create local folder */
        if (toolbox::FileSystemHelper::folderExists(".simfs")) {
            LOG(INFO, 0, "SimFS is already initialized here: overwriting!");
        }
        else if (toolbox::FileSystemHelper::mkDir(".simfs")) {
            LOG(ERROR, 0, "Failed to initialize SimFS!\n");
            return -1;
        }

        /* copy conf file */
        toolbox::FileSystemHelper::cpFile(conf_file, DIR_SIMFS + std::string("/") + CONF_NAME);
 
        /* save key in the workspace */        
        string current_path = FileSystemHelper::getCwd();
        ws.setString(name, current_path);
        simfs::saveWorkspace(ws);

        return 0;    
    }
}


void error_exit(const std::string &name, const std::string &additional_text) {
	std::cout << "Usage: " << name << " <command> [OPTIONS]" << std::endl;
	std::cout << "\t" << name << " help for list of OPTIONS" << std::endl;
	std::cout << std::endl;
    LOG(ERROR, 0, additional_text);
	exit(1);
}

void initLogger(){

    /* Note: this order has to reflect the macro numbering! (see below)*/
    const char * keynames[] = {"ERROR", "INFO", "SIMULATOR", "CLIENT", "CACHE", "PREFETCHER", "WARNING", NULL};

    char * log_keys = getenv("SIMFS_LOG");
    char * log_level_str = getenv("SIMFS_LOG_LEVEL");

    int log_level = log_level_str==NULL? 0 : atoi(log_level_str);

    Logger::setLogKey(ERROR, 10, keynames[0], LOG_FULL, ANSI_COLOR_RED);
    Logger::setLogKey(INFO, -1, keynames[1], LOG_SIMPLE, ANSI_COLOR_YELLOW);
    Logger::setLogKey(SIMULATOR, -1, keynames[2], LOG_SIMPLE, ANSI_COLOR_BLUE);
    Logger::setLogKey(CLIENT, -1, keynames[3], LOG_SIMPLE, ANSI_COLOR_GREEN);
    Logger::setLogKey(CACHE, -1, keynames[4], LOG_SIMPLE, ANSI_COLOR_YELLOW);
    Logger::setLogKey(PREFETCHER, -1, keynames[5], LOG_SIMPLE, ANSI_COLOR_MAGENTA);
    Logger::setLogKey(WARNING, -1, keynames[6], LOG_SIMPLE, ANSI_COLOR_YELLOW);
    
    if (log_keys!=NULL) {
        std::string keys_str = std::string(log_keys);
        const char ** keyptr = keynames; 


        
        while (*keyptr!=NULL){
            if (keys_str.find(std::string(*keyptr))!=std::string::npos){
                /* Strong assumption about jeynames, MACRO ordering/numbering! */
                Logger::setLogKeyLevel(ERROR + (keyptr-keynames), log_level);
            }
            keyptr++;
        }
    }
}

int main(int argc, char * argv[]){

    /* Let GDB in */
    int attach_gdb = getenv("DV_ATTACH_GDB")!=NULL;
    if (attach_gdb){
        gdbstop=1;
        std::cout << "Waiting for GDB (PID: " << getpid() << "; type 'set gdbstop=0' to continue)." << std::endl;
        while (gdbstop){;}
    }

    /* Load SimFS workspace */
    KeyValueStore ws;
    if (!simfs::loadWorkspace(ws)){
        LOG(ERROR, 0, "Failed the load SimFS configuration!");
        exit(-1);
    }

    /* Init logger */
    initLogger();

    /* Parsing & checking arguments */
	if (argc < 2) {
		error_exit(argv[0], "A command is expected!");
	}


    std::string cmd(argv[1]);    

    
    /* Executing commands*/
    if (cmd == SIMFS_INIT){

        if (argc!=4) {
            error_exit(argv[0], "Invalid syntax. See usage.");
        }
        
        std::string env = argv[2];
        std::string conf_file = argv[3];

        simfs::initEnvirorment(ws, env, conf_file);

    }else if (cmd == SIMFS_RUN){
        if (argc<3){
            error_exit(argv[0], "Invalid syntax. See usage.");
        }

        std::string env = argv[2];
        if (!ws.hasKey(env)){ error_exit(argv[0], "This environment does not exit!"); }
        std::string conf_file = ws.getString(env) + CONF_NAME;

        const char * libname = getenv("SIMFS_DVLIB");
        if (libname==NULL) libname = DEFAULT_DVLIB;

        printf("SimFS envirorment: %s\n", env.c_str());

        setenv("LD_PRELOAD", (SIMFS_DVLIB + string(libname)).c_str(), 1);

        /* Check if there is a known IP/PORT for it */
        if (ws.hasKey(IPKEY(env)) && ws.hasKey(PORTKEY(env))){
            string ip = ws.getString(IPKEY(env));
            string port = ws.getString(PORTKEY(env));
            
            printf("DV should be listening on %s:%s\n", ip.c_str(), port.c_str());

            setenv("DV_PROXY_SRV_IP", ip.c_str(), 1);
            setenv("DV_PROXY_SRV_PORT", port.c_str(), 1);
        }
    
        execvpe(argv[3], &(argv[3]), environ);

       
    }else if (cmd == SIMFS_START){

	    gengetopt_args_info args_info;
    
        if (argc<3){
            error_exit(argv[0], "SimFS envirorment not specified!");
        }
    
        std::string env(argv[2]);     
        argv[2] = argv[0];

        /* Parse DV options */
        cmdline_parser(argc-2, &(argv[2]), &args_info);
    	if (cmdline_parser(argc, argv, &args_info) != 0) {
		    error_exit(argv[0], "Error while parsing command line arguments");
	    }

        /* Load env */
        if (!ws.hasKey(env)){ error_exit(argv[0], "This environment does not exit!"); }
        std::string conf_file = ws.getString(env) + "/" + DIR_SIMFS + "/" + CONF_NAME;
       
        DV * dv = DVCreate(args_info, conf_file);
        if (dv==NULL){ exit(1); }
 
        /* Save the IP/PORT on which we are listening */
        std::string ip = dv->getIpAddress();
        std::string port = dv->getClientPort();        
   
        ws.setString(IPKEY(env), ip);
        ws.setString(PORTKEY(env), port);               
        simfs::saveWorkspace(ws);


        dv->run(); 
        delete dv;

	    cmdline_parser_free(&args_info);
    }




    /* Cleaning up */

	return 0;
}

