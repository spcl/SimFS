#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sstream>
#include <string.h>
#include "config.h"

#include "getopt/dv_cmdline.h"
#include "server/DV.h"
#include "dv.h"
#include "SimFS.hpp"
#include "SimFSEnv.hpp"
#include "toolbox/StringHelper.h"
#include "toolbox/FileSystemHelper.h" 

/* AF_INET socket stuff */
#include <arpa/inet.h> 
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>


#define ADDRESS_DELIM ","

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

std::string findWorkingAddress(const std::string addresses, const std::string port){
   
    struct sockaddr_in srv_addr;
    uint16_t srv_port = htons(atoi(port.c_str()));
    
    std::vector<std::string> address_vec;
    StringHelper::splitStr(&address_vec, addresses, ADDRESS_DELIM);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    for (auto it = address_vec.begin(); it != address_vec.end(); ++it){

        if (*it == "") continue; 
        const char * srv_ip = (*it).c_str();
        
        memset(&srv_addr, '0', sizeof(srv_addr));
        srv_addr.sin_family = AF_INET;
        srv_addr.sin_port = srv_port;
        int res = inet_pton(AF_INET, srv_ip, &srv_addr.sin_addr);

        if (res<=0) {
            sockfd=0;
            printf("%s is an invalid IP address\n", srv_ip);
        }else{

            printf("Connecting to %s:%s\n", srv_ip, port.c_str());
            res = -1;
            res = connect(sockfd, (struct sockaddr *)&srv_addr, sizeof(srv_addr));
            if (res<0) {
                printf("Cannot connect to: %i (%i)\n", res, res);
                perror("Error");
            }else{
                close(sockfd);
                printf("Found a DV listening on: %s\n", srv_ip);
                return *it;
            }
        }
    }
    return "";
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

            printf("Candidate IPs for DV: %s; port: %s\n", addr.ip.c_str(), addr.port.c_str());

            std::string dv_working_ip = findWorkingAddress(addr.ip, addr.port);

            if (dv_working_ip=="") return -1;

            setenv("DV_PROXY_SRV_IP", dv_working_ip.c_str(), 1);
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

        /* load env */
        SimFSEnv env(fs, envname);
        if (!env.isValid()){
            error_exit(argv[0], "error while initializing the environment!");
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

        SimFSEnv env(fs, envname);
        if (!env.isValid()){
            error_exit(argv[0], "error while initializing the environment!");
        }   

        KeyValueStore fileidx;
        env.loadFiles(&fileidx);
        
        //get env output path
        DVConfig dvconf;
        if (!dvconf.loadConfigFile(env.getConfigFile())){
            error_exit(argv[0], "Cannot read the config file " + env.getConfigFile());
        }

        if (!dvconf.init()){
            error_exit(argv[0], "Configuration is invalid (file: " + env.getConfigFile() + ")");
        }       

        std::string env_output = FileSystemHelper::getRealPath(dvconf.sim_result_path_);       
        std::string cwd = FileSystemHelper::getCwd();    
        
        if (!cwd.compare(0, env_output.size(), env_output)){

            std::unordered_map<std::string, std::string> files = fileidx.getStoreMap();
            std::string reldir = cwd.substr(env_output.length());
            if (reldir=="") reldir="/";

            //std::cout << "rel dir: " << reldir << std::endl;
            for (auto f = files.begin(); f != files.end(); ++f){
                std::string fdir = FileSystemHelper::getDirname(f->first);
                //std::cout << "fdir: " << fdir << std::endl;
                if (fdir==reldir){
                    printf("%s\n", FileSystemHelper::getBasename(f->first).c_str());
                }

            }
        }else{
            error_exit(argv[0], "This is not an the output dir of this context!");
        }

    } else if (cmd == SIMFS_INDEX){
        
        /* load env */
        SimFSEnv env(fs, envname);
        if (!env.isValid()){
            error_exit(argv[0], "error while initializing the environment!");
        }   

        //get filename (cannot get realpath here since it may not exist)
        std::string file = argv[3];

        //get absolute path
        std::string dirname = FileSystemHelper::getDirname(file);
        std::string filename = FileSystemHelper::getBasename(file);

        KeyValueStore fileidx;
        env.loadFiles(&fileidx);
        
        //get env output path
        DVConfig dvconf;
        if (!dvconf.loadConfigFile(env.getConfigFile())){
            error_exit(argv[0], "Cannot read the config file " + env.getConfigFile());
        }

        if (!dvconf.init()){
            error_exit(argv[0], "Configuration is invalid (file: " + env.getConfigFile() + ")");
        }       

        std::string env_output = FileSystemHelper::getRealPath(dvconf.sim_result_path_);       
        std::cout << "file: " << file<< "; dir: " << dirname << "; env output: " << env_output << std::endl;
        

        if (!dirname.compare(0, env_output.size(), env_output)){
             
            std::string reldir = dirname.substr(env_output.length());
            bool file_exists = FileSystemHelper::fileExists(file);

            std::string filename_idx = reldir + "/" + filename;
            std::cout << "indexing file " << filename_idx << "; exists: " << file_exists << std::endl;

            fileidx.setString(filename_idx, file_exists ? "1" : "0");
            if (!env.saveFiles(&fileidx)){
                error_exit(argv[0], "Error while saving the index!\n");
            }        

        }else{
            std::cout << "This file is not in the output directory of this environment! (directory of the file: " << dirname << ";  env. output dir: " << env_output << std::endl;
        }


    } else if (cmd == SIMFS_INFO){


    } else {
        printf("Command not recognized!\n");
        exit(1);
    }


    /* Cleaning up */

    return 0;
}

