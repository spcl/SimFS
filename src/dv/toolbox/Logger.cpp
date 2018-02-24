#include "Logger.h"
#include <stdio.h>
#include <string>

#include "FileSystemHelper.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"


namespace toolbox{

    
    const char * Logger::log_key_names_[] = {KEYNAMES};
    std::string Logger::tolog_="";
    int Logger::level_=0;

    void Logger::setLogKeys(const std::string &tolog, int level){
        tolog_ = tolog;
        level_ = level;
    }

    bool Logger::toLog(int key){
        return tolog_=="all" or tolog_.find(log_key_names_[key])!=std::string::npos;
    }

    int Logger::log(int key, int level, const std::string &msg, const std::string &file, int line){
        const char * color_start = ANSI_COLOR_RESET;
        if (toLog(key) && level<=level_){
            if (key==INFO || key==WARNING){ color_start = ANSI_COLOR_YELLOW; }
            else if (key==ERROR) { color_start = ANSI_COLOR_RED; }        
            else if (key==CLI_HANDLER) { color_start = ANSI_COLOR_MAGENTA; }
            else if (key==SIM_HANDLER) { color_start = ANSI_COLOR_BLUE; }   
            else if (key==CACHE) { color_start = ANSI_COLOR_CYAN; }

#ifdef DEBUG
            printf("%s%s:%d: %s%s\n", color_start, FileSystemHelper::getBasename(file).c_str(), line, msg.c_str(), ANSI_COLOR_RESET);
#else
            printf("%s%s: %s%s\n", color_start, log_key_names_[key], msg.c_str(), ANSI_COLOR_RESET);
#endif
            return 0;
        }
        return 1;
    }
}
