/*------------------------------------------------------------------------------
 * CppToolbox: FileSystemHelper
 * https://github.com/pirminschmid/CppToolbox
 *
 * Copyright (c) 2017, Pirmin Schmid, MIT license
 *----------------------------------------------------------------------------*/

#include "FileSystemHelper.h"

#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <vector>


namespace toolbox {

	bool FileSystemHelper::acceptAll(const std::string &name) {
		return true;
	}

	void FileSystemHelper::readDir(const std::string &path,
								   FileSystemHelper::handle_file_type callback,
								   bool include_sub_dirs,
								   FileSystemHelper::predicate_type accept_function) {

		readDirRecursive(path, callback, include_sub_dirs, accept_function, "");
	}

	void FileSystemHelper::readDirRecursive(const std::string &path,
								   FileSystemHelper::handle_file_type callback,
								   bool include_sub_dirs,
								   FileSystemHelper::predicate_type accept_function,
								   const std::string &sub_dir) {

		std::string checked_path = (path.length() == 0) ?
								   path : (path.back() == '/') ?
										  path : path + "/";

		std::string checked_sub_dir = (sub_dir.length() == 0) ?
									  sub_dir : (sub_dir.back() == '/') ?
												sub_dir : sub_dir + "/";

		DIR *dir_ptr;
		struct dirent *dir_entry_ptr;

		std::vector<std::pair<std::string, std::string>> sub_folders;

		dir_ptr = opendir(checked_path.c_str());
		if (dir_ptr != nullptr) {
			while ((dir_entry_ptr = readdir(dir_ptr)) != nullptr) {
				if (dir_entry_ptr->d_name[0] == '.') {
					// no hidden files
					// no . and .. folders
					continue;
				}

				std::string name = dir_entry_ptr->d_name;
				std::string rel_path = checked_sub_dir + name;
				std::string full_path = checked_path + name;

				if (dir_entry_ptr->d_type == DT_REG) {
					if (accept_function(name)) {
						callback(name, rel_path, full_path);
					}
				} else if (dir_entry_ptr->d_type == DT_DIR) {
					if (include_sub_dirs) {
						sub_folders.push_back(std::make_pair(full_path, rel_path));
					}
				}
			}

			closedir(dir_ptr);
		}

		// recursive calls here, to avoid having lots of dirs open
		for (const auto &next : sub_folders) {
			readDirRecursive(next.first, callback, include_sub_dirs, accept_function, next.second);
		}

	}


	bool FileSystemHelper::fileExists(const std::string &path) {
		struct stat buffer;
		int ret = stat(path.c_str(), &buffer);
		if (ret != 0) {
			return false;
		}

		if (S_ISREG(buffer.st_mode)) {
			return true;
		}

		return false;
	}


	bool FileSystemHelper::folderExists(const std::string &path) {
		struct stat buffer;
		int ret = stat(path.c_str(), &buffer);
		if (ret != 0) {
			return false;
		}

		if (S_ISDIR(buffer.st_mode)) {
			return true;
		}

		return false;
	}


	int64_t FileSystemHelper::fileSize(const std::string &path) {
		struct stat buffer;
		int ret = stat(path.c_str(), &buffer);
		if (ret != 0) {
			return -1;
		}

		return static_cast<int64_t>(buffer.st_size);
	}

	void FileSystemHelper::rmFile(const std::string &path) {
		std::remove(path.c_str());
	}

	std::string FileSystemHelper::getBasename(const std::string &path) {
		// note basename may modify the string -> create a copy first
		char buffer[path.length() + 2];
		strcpy(buffer, path.c_str());
		char *b = basename(buffer);
		if (b == nullptr) {
			return std::string("");
		}
		return std::string(b);
	}

	std::string FileSystemHelper::getDirname(const std::string &path) {
		// note dirname may modify the string -> create a copy first
		char buffer[path.length() + 2];
		strcpy(buffer, path.c_str());
		char *b = dirname(buffer);
		if (b == nullptr) {
			return std::string("");
		}
		return std::string(b);
	}
}
