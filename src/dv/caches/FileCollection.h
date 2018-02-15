//
// Created by Pirmin Schmid on 17.04.17.
//

#ifndef DV_CACHES_FILECOLLECTION_H_
#define DV_CACHES_FILECOLLECTION_H_

#include <memory>
#include <string>

#include "../DVForwardDeclarations.h"
#include "../DVBasicTypes.h"

namespace dv {

	/**
	 * Abstract base class / interface to hold some file descriptors.
	 * Used by FileSystemHelper::readDir() to load files into caches
	 * and restart files into collections for additional processing
	 *
	 * note: it only defines put() and getStats(); access operations are sub-class specific.
	 */
	class FileCollection {
	public:
		struct Stats {
			dv::counter_type count_all;
			dv::counter_type count_evictable;
			dv::size_type filesize_all;
			dv::size_type filesize_evictable;
		};

		FileCollection() {};

		virtual ~FileCollection() {};

		virtual void put(const std::string &key, std::unique_ptr<FileDescriptor> value) = 0;

		virtual Stats getStats() const = 0;
	};

}

#endif //DV_CACHES_FILECOLLECTION_H_
