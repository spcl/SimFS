#include "toolbox/FileSystemHelper.h"
#include "toolbox/Logger.h"
#include "getopt/dv_cmdline.h"
#include "dv.h"

#include "server/DV.h"

#include <sys/types.h>
#include <unistd.h>


#define DIR_SIMFS ".simfs"

/* commands */
#define SIMFS_INIT "init"
#define SIMFS_START "start"
#define SIMF_LS "ls"
#define SIMFS_INDEX "index"


using namespace dv;
using namespace toolbox;
using namespace std;

volatile int gdbstop;

namespace simfs { 

    string find_conf(){
        string current_path = FileSystemHelper::getCwd();
        while (!FileSystemHelper::folderExists(current_path + "/" + DIR_SIMFS) && current_path!="/"){
            current_path = FileSystemHelper::getDirname(current_path);
        }

        if (current_path=="/"){
            LOG(ERROR, "This is not a SimFS envirorment!");
            return "";
        }
               
        string conf_file = current_path + "/" + DIR_SIMFS + "/conf.dv";
        
        if (!FileSystemHelper::fileExists(conf_file)) {
            LOG(ERROR, "SimFS configuration file not found!");
            return "";
        }
        
        LOG(INFO, "Configuration file found in: " + conf_file);

        return conf_file;
    }

    int init(char * conf_file){
        if (toolbox::FileSystemHelper::mkDir(".simfs")) {
            printf("Failed to initialize simfs dir!\n");
            return -1;
        }
      
        if (conf_file!=NULL) toolbox::FileSystemHelper::cpFile(conf_file, DIR_SIMFS"/conf.dv");
 
        return 0;    
    }
}


void error_exit(const std::string &name, const std::string &additional_text) {
	std::cout << "Usage: " << name << " <command> [OPTIONS]" << std::endl;
	std::cout << "\t" << name << " help for list of OPTIONS" << std::endl;
	std::cout << std::endl;
    LOG(ERROR, additional_text);
	exit(1);
}


int main(int argc, char * argv[]){

	gengetopt_args_info args_info;

    /* Let GDB in */
    int attach_gdb = getenv("DV_ATTACH_GDB")!=NULL;
    if (attach_gdb){
        gdbstop=1;
        std::cout << "Waiting for GDB (PID: " << getpid() << "; type 'set gdbstop=0' to continue)." << std::endl;
        while (gdbstop){;}
    }

    /* Parsing & checking arguments */
	if (argc < 2) {
		error_exit(argv[0], "A command is expected!");
	}

    std::string cmd(argv[1]);    
    argv[1] = argv[0];

    cmdline_parser(argc-1, &(argv[1]), &args_info);

	if (cmdline_parser(argc, argv, &args_info) != 0) {
		error_exit(argv[0], "Error while parsing command line arguments");
	}

    /* Executing commands*/
    if (cmd == SIMFS_INIT){
        simfs::init(args_info.conf_file_given ? args_info.conf_file_arg : NULL);
    }else if (cmd == SIMFS_START){
        std::string conf_file = simfs::find_conf();
        if (conf_file != "") { 
            DV * dv = DVCreate(args_info, conf_file);
            if (dv==NULL){ exit(1); }
            else { 
                dv->run(); 
                delete dv;
            }
        }
    }

    /* Cleaning up */
	cmdline_parser_free(&args_info);

	return 0;
}

