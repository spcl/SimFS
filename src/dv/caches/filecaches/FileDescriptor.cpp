//
// Created by Pirmin Schmid on 12.04.17.
//

#include "FileDescriptor.h"
#include "../../server/ClientDescriptor.h"

#include <cmath>
#include <iostream>

namespace dv {

FileDescriptor::FileDescriptor(const std::string &name, const std::string &file_name) :
    name_(name), file_name_(file_name) {}

const std::string &FileDescriptor::getName() const {
    return name_;
}

const std::string &FileDescriptor::getFileName() const {
    return file_name_;
}

void FileDescriptor::setPartitionKey(dv::id_type partition_key) {
    partition_key_ = partition_key;
}

dv::id_type FileDescriptor::getPartitionKey() const {
    return partition_key_;
}

void FileDescriptor::setNr(dv::id_type id_nr) {
    id_nr_ = id_nr;
}

dv::id_type FileDescriptor::getNr() const {
    return id_nr_;
}

dv::counter_type FileDescriptor::getUseCount() const {
    return use_count_;
}

dv::counter_type FileDescriptor::getLockCount() const {
    return lock_count_;
}

bool FileDescriptor::isFileAvailable() const {
    return file_available_;
}

void FileDescriptor::setFileAvailable(bool available) {
    file_available_ = available;
}

bool FileDescriptor::isFileUsedBySimulator() const {
    return 0 < file_used_by_simulator_count_;
}

void FileDescriptor::setFileUsedBySimulator(bool used) {
    if (used) {
        ++file_used_by_simulator_count_;
    } else {
        if (0 < file_used_by_simulator_count_) {
            --file_used_by_simulator_count_;
        }
    }
}

bool FileDescriptor::isFilePrefetched() const {
    return is_prefetched_;
}

void FileDescriptor::setFilePrefetched(bool prefetched) {
    is_prefetched_ = prefetched;
}

void FileDescriptor::setSize(dv::size_type size) {
    size_ = size;
}

dv::size_type FileDescriptor::getSize() const {
    return size_;
}

void FileDescriptor::setCost(dv::cost_type cost) {
    cost_ = cost;
}

void FileDescriptor::setUnitCost() {
    cost_ = kUnitCost;
}

void FileDescriptor::applyPenaltyFactor(double penalty_factor) {
    cost_ = static_cast<dv::cost_type >(std::floor(penalty_factor * static_cast<double>(cost_)));
}

dv::cost_type FileDescriptor::getActualCost() const {
    if (0 < use_count_) {
        return cost_;
    } else {
        if (0 == cost_) {
            return 0;
        }

        //return 0;
        dv::cost_type reduced = cost_ >> 1;
        if (0 < reduced) {
            return reduced;
        } else {
            return 1;
        }
    }
}

void FileDescriptor::lock() {
    ++lock_count_;
    ++use_count_;
}

void FileDescriptor::unlock() {
    if (0 < lock_count_) {
        --lock_count_;
    } else {
        std::cerr << "FileDescriptor " << name_ << ": ERROR: unlock() before lock()" << std::endl;
    }
}

void FileDescriptor::appendNotificationSocket(int socket) {
    notification_sockets_.emplace(socket);
}

void FileDescriptor::removeAllNotificationSockets() {
    notification_sockets_.clear();
}

const std::unordered_set<int> &FileDescriptor::getNotificationSockets() const {
    return notification_sockets_;
}

void FileDescriptor::appendWaitingClientPtr(ClientDescriptor *client_descriptor_ptr) {
    waiting_clients_ptrs_.emplace(client_descriptor_ptr);
}

void FileDescriptor::removeAllWaitingClientPtrs() {
    waiting_clients_ptrs_.clear();
}

const std::unordered_set<ClientDescriptor *> &FileDescriptor::getWaitingClientPtrs() const {
    return waiting_clients_ptrs_;
}

void FileDescriptor::addVariable(const std::string &id, size_t offset, size_t count) {
    auto r = variables_.find(id);
    if (r == variables_.end()) {
        variables_.emplace(std::make_pair(id, VariableDescriptor(id, offset, count)));
    } else {
        r->second.extendWithIndices(offset, count);
    }
}

std::string FileDescriptor::toString() const {
    return "FileDescriptor " + name_ + ": available " + std::to_string(file_available_) + " lock count " + std::to_string(lock_count_) + " file name " + file_name_;
}
}
