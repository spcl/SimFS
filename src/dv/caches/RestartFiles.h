//
// Created by Pirmin Schmid on 17.04.17.
//

#ifndef DV_CACHES_RESTARTFILES_H_
#define DV_CACHES_RESTARTFILES_H_

#include <string>
#include <vector>

#include "FileCollection.h"
#include "filecaches/FileDescriptor.h"

namespace dv {

	class RestartFiles : public FileCollection {
	public:
		virtual void put(const std::string &key, std::unique_ptr<FileDescriptor> value) override;

		virtual Stats getStats() const override;

		std::vector<FileDescriptor *> getAllFiles(); // not const since pointer objects can be modified

	private:
		std::vector<std::unique_ptr<FileDescriptor>> files_;
		dv::size_type total_file_size_ = 0;
	};

}

#endif //DV_CACHES_RESTARTFILES_H_
