#include "Logger.h"
#include <stdio.h>
#include <string>

#include "FileSystemHelper.h"


namespace toolbox {

bool Logger::log_all_ = false;
std::unordered_map<int, Logger::logkey> Logger::keymap_;



void Logger::setLogKey(int key, int level, const char * name, int format, const char * color) {
    struct logkey lg;
    lg.name = name;
    lg.format = format;
    lg.color = color;
    lg.level = level;

    keymap_[key] = lg;
}

void Logger::setLogKeyLevel(int key, int level) {
    if (keymap_.find(key)==keymap_.end()) return;
    keymap_[key].level = level;
}

void Logger::logAll(bool logall) {
    log_all_ = logall;
}

int Logger::log(int key, int level, const std::string &msg, const std::string &file, int line) {
    const char * color_start = ANSI_COLOR_RESET;

    if (log_all_ || (keymap_.find(key) != keymap_.end() && keymap_[key].level >= level)) {

        color_start = keymap_[key].color;

        /*
        if (key==INFO || key==WARNING){ color_start = ANSI_COLOR_YELLOW; }
        else if (key==ERROR) { color_start = ANSI_COLOR_RED; }
        else if (key==CLIENT) { color_start = ANSI_COLOR_GREEN; }
        else if (key==SIMULATOR) { color_start = ANSI_COLOR_BLUE; }
        else if (key==CACHE) { color_start = ANSI_COLOR_YELLOW; }
        else if (key==PREFETCHER) { color_start = ANSI_COLOR_MAGENTA; }
        */

        if (keymap_[key].format==LOG_FULL) {
            printf("%s%s (%s:%d): %s%s\n", color_start, keymap_[key].name, FileSystemHelper::getBasename(file).c_str(), line, msg.c_str(), ANSI_COLOR_RESET);
        } else {
            printf("%s%s: %s%s\n", color_start, keymap_[key].name, msg.c_str(), ANSI_COLOR_RESET);
        }

        return 0;
    }
    return 1;
}
}
