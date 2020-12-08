#ifndef _FILE_UTIL_H_
#define _FILE_UTIL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <errno.h>

//class CFileUtil {
//public:
//	static bool mkdirs(char *path);
//	static bool isDirectory(const char *path);
//	static bool isSymLink(const char *paht);
//	static bool exist(const char *path);
//};

//namespace YaoUtil {

	class PathUtil
	{
	public:
		static std::string ModuleFileDirectory_();

		static std::string GetDirectory_(const std::string& fileFullPath);

		static std::string GetFile(const std::string& fileFullPath);

		// directory+folder or directory+file
		static std::string PathCombine_(const std::string& path1, const std::string& path2);

		// path_ -> directory or fileFullPath
		static bool exist(const std::string& path_);

		static bool IsStrJustFileName_(const std::string& str);

		static void CreateFolder_(const std::string& directory_);
	};

//}//end YaoUtil namespace

uint64_t get_tick_count();
void util_sleep(uint32_t millisecond);



#endif /* _FILE_UTIL_H_ */
