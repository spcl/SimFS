//
// Created by Pirmin Schmid on 18.04.17.
//

#ifndef DV_CACHES_FILECACHES_FILECACHEUNLIMITED_H_
#define DV_CACHES_FILECACHES_FILECACHEUNLIMITED_H_

#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../DVForwardDeclarations.h"
#include "FileCache.h"
#include "FileDescriptor.h"


namespace dv {

	class FileCacheUnlimited : public FileCache {
	public:
		FileCacheUnlimited(DV *dv_ptr);

		virtual void initializeWithFiles() override;

		virtual void put(const std::string &key, std::unique_ptr<FileDescriptor> value) override;

		virtual FileDescriptor *get(const std::string &key) override;

		virtual FileDescriptor *internal_lookup_get(const std::string &key) override;

		virtual void refresh(const std::string &key) override;

		virtual const std::vector<std::string> &getAccessTrace() const override;

		virtual const std::string &name() const override;

		virtual dv::id_type capacity() const override;

		virtual dv::id_type size() const override;

		virtual Stats getStats() const override;

		virtual void printStatus(std::ostream *out) override;

		virtual const toolbox::KeyValueStore &getStatusSummary() override;

	private:
		static constexpr char kCacheName[] = "LRU cache: ";

		DV *dv_ptr_;
		std::unordered_map<std::string, std::unique_ptr<FileDescriptor>> files_;
		std::vector<std::string> access_trace_;
		dv::size_type total_file_size_ = 0;

		std::string cache_name_;

		toolbox::KeyValueStore statusSummary_;
	};

}

#endif //DV_CACHES_FILECACHES_FILECACHEUNLIMITED_H_
