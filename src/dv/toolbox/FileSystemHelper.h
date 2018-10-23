/*------------------------------------------------------------------------------
 * CppToolbox: FileSystemHelper
 * https://github.com/pirminschmid/CppToolbox
 *
 * Copyright (c) 2017, Pirmin Schmid, MIT license
 *----------------------------------------------------------------------------*/

#ifndef TOOLBOX_FILESYSTEMHELPER_H_
#define TOOLBOX_FILESYSTEMHELPER_H_

#include <functional>
#include <string>

#define PATH_LEN 2048


namespace toolbox {

	/**
	 * some convenience file system methods. Limited to POSIX file systems at the moment.
	 * Prefer TS Filesystem that comes with C++17 if working with C++17.
	 */
	class FileSystemHelper {
	public:

		typedef std::function<bool(const std::string &name)> predicate_type;

		/**
		 * default predicate function
		 * @param name
		 * @return true for all names
		 */
		static bool acceptAll(const std::string &name);


		/**
		 * arguments: name, relative path from given path, full path including given path
		 * note: full path is based on given input path. No additional absolute path resolution!
		 */
		typedef std::function<void(const std::string &name,
								   const std::string &rel_path,
								   const std::string &full_path)> handle_file_type;

		/**
		 * @param path
		 * @param callback
		 * @param include_sub_dirs
		 * @param accept_function  predicate to select files for inclusion
		 */
		static void readDir(const std::string &path,
							handle_file_type callback,
							bool include_sub_dirs,
							predicate_type accept_function = FileSystemHelper::acceptAll);


		/**
		 * A simple test for file existence.
		 * Uses stat().
		 *
		 * @param path
		 * @return true/false
		 */
		static bool fileExists(const std::string &path);


		/**
 		 * A simple test for folder existence.
 		 * Uses stat().
 		 *
 		 * @param path
 		 * @return true/false
 		 */
		static bool folderExists(const std::string &path);


		/**
		 * Returns file size as determined by stat().
		 *
		 * @param path
		 * @return file size in bytes, or -1 in case of an error while calling stat().
		 */
		static int64_t fileSize(const std::string &path);


		/**
		 * removes a file in filesystem
		 * @param path
		 */
		static void rmFile(const std::string &path);


		/**
		 * @return file realpath
		 */
        static std::string getRealPath(const std::string &path);


		/**
		 * @return file basename of path
		 */
		static std::string getBasename(const std::string &path);


		/**
 		 * @return folder part of path
 		 */
		static std::string getDirname(const std::string &path);


		/**
 		 * creates the folder specified by path with read/write/search 
         * permissions for owner and group, and with read/search permissions 
         * for others).
         * @return 0 if success, -1 otherwise (errno is set)
 		 */
        static int mkDir(const std::string &path);
        
        /**
         * copies a file from origin to dest 
         */
        static void cpFile(const std::string &origin, const std::string &dest);


        /**
         * get the current working directory 
         */
        static std::string getCwd();

	private:
		static void readDirRecursive(const std::string &path,
							handle_file_type callback,
							bool include_sub_dirs,
							predicate_type accept_function,
							const std::string &sub_dir);
	};

}

#endif //TOOLBOX_FILESYSTEMHELPER_H_
