//
// Created by Pirmin Schmid on 12.04.17.
//

#ifndef DV_CACHES_FILECACHES_VARIABLEDESCRIPTOR_H
#define DV_CACHES_FILECACHES_VARIABLEDESCRIPTOR_H

#include <ostream>
#include <string>

namespace dv {

	/**
	 * not yet really implemented. We may think about redesigning it to allow better use for netcdf and hdf5.
	 */
	class VariableDescriptor {
	public:
		VariableDescriptor();

		VariableDescriptor(const std::string &id, size_t offset, size_t count);

		void extendWithIndices(size_t offset, size_t count);

		void extendWithDescriptor(const VariableDescriptor &other);

		bool contains(size_t offset, size_t count);

		void print(std::ostream *out);

	private:
		std::string id_;
		size_t offset_;
		size_t count_;
	};

}

#endif //DV_CACHES_FILECACHES_VARIABLEDESCRIPTOR_H
