#include "Logger.h"
#include <stdio.h>

#include "FileSystemHelper.h"

namespace toolbox{

    std::string Logger::tolog_="";

    void Logger::setLogKeys(const std::string &tolog){
        tolog_ = tolog;
    }

    bool Logger::toLog(const std::string &key){
        return tolog_=="" || tolog_.find(key)!=std::string::npos;
    }

    int Logger::log(const std::string &key, const std::string &msg, const std::string &file, int line){
        if (toLog(key)){
            printf("%s:%d: %s\n", FileSystemHelper::getBasename(file).c_str(), line, msg.c_str());
            return 0;
        }
        return 1;
    }
}
