//
// Created by Pirmin Schmid on 12.04.17.
//

#ifndef DV_CACHES_FILECACHES_FILECACHE_H_
#define DV_CACHES_FILECACHES_FILECACHE_H_

#include <memory>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../DVForwardDeclarations.h"
#include "../FileCollection.h"
#include "../../toolbox/KeyValueStore.h"
#include "../../DVBasicTypes.h"

namespace dv {

	class FileCache : public FileCollection {
	public:

		FileCache() {};

		virtual ~FileCache() {};

		virtual void initializeWithFiles() = 0;

		// put: abstract definition in FileCollection

		virtual FileDescriptor *get(const std::string &key) = 0;

		virtual FileDescriptor *internal_lookup_get(const std::string &key) = 0;

		virtual void refresh(const std::string &key) = 0;

		virtual const std::vector<std::string> &getAccessTrace() const = 0;

		virtual const std::string &name() const = 0;

		virtual dv::id_type capacity() const = 0;

		virtual dv::id_type size() const = 0;

		// getStats: abstract definition in FileCollection

		virtual void printStatus(std::ostream *out) = 0;

		virtual const toolbox::KeyValueStore &getStatusSummary() = 0;

		enum FileCacheType { kUndefinedFileCache, kUnlimited, kLRU, kARC, kLIRS, kBCL, kDCL, kACL, kPLRU, kPBCL, kPDCL, kPACL };

		static FileCacheType getFileCacheType(const std::string &s) {
			std::unordered_map<std::string, FileCacheType> cache_switcher{
					{"unlimited", kUnlimited},
					{"LRU", kLRU},
					{"ARC", kARC},
					{"LIRS", kLIRS},
					{"BCL", kBCL},
					{"DCL", kDCL},
					{"ACL", kACL},
					{"PLRU", kPLRU},
					{"PBCL", kPBCL},
					{"PDCL", kPDCL},
					{"PACL", kPACL},
			};

			auto r = cache_switcher.find(s);
			if (r == cache_switcher.end()) {
				return kUndefinedFileCache;
			}

			return r->second;
		}

	};

}

#endif //DV_CACHES_FILECACHES_FILECACHE_H
