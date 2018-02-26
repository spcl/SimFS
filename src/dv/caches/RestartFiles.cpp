//
// Created by Pirmin Schmid on 17.04.17.
//

#include "RestartFiles.h"

namespace dv {

void RestartFiles::put(const std::string &key, std::unique_ptr<FileDescriptor> value) {
    total_file_size_ += value->getSize();
    files_.push_back(std::move(value));
}

FileCollection::Stats RestartFiles::getStats() const {
    Stats result = {};
    result.count_all = files_.size();
    result.filesize_all = total_file_size_;
    return result;
}

std::vector<FileDescriptor *> RestartFiles::getAllFiles() {
    std::vector<FileDescriptor *> file_ptrs;
    file_ptrs.reserve(files_.size());
    for (const auto &f : files_) {
        file_ptrs.push_back(f.get());
    }
    return file_ptrs;
}

}
