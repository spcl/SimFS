//
// Created by Pirmin Schmid on 12.04.17.
//

#include "VariableDescriptor.h"


namespace dv {

VariableDescriptor::VariableDescriptor() : id_(""), offset_(0), count_(0) {}

VariableDescriptor::VariableDescriptor(const std::string &id, const size_t offset, const size_t count) :
    id_(id), offset_(offset), count_(count) {
}

void VariableDescriptor::extendWithIndices(size_t offset, size_t count) {
    // TODO
}

void VariableDescriptor::extendWithDescriptor(const VariableDescriptor &other) {
    // TODO
}

bool VariableDescriptor::contains(size_t offset, size_t count) {
    // TODO
    return true;
}

void VariableDescriptor::print(std::ostream *out) {
    *out << "VariableDescriptor " << id_ << ": offset " << offset_ << " count " << count_ << std::endl;
}
}