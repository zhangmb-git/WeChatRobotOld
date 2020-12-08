#include "file_util.h"
#include <Shlwapi.h>
#include <ShlObj_core.h>

#ifndef S_IRWXUGO
#define S_IRWXUGO    (S_IRWXU | S_IRWXG | S_IRWXO)
#endif

std::string PathUtil::ModuleFileDirectory_()
{
#ifdef _WIN32
	char buf[MAX_PATH] = { 0 };
	::GetModuleFileNameA(NULL, buf, MAX_PATH);
	::PathRemoveFileSpecA(buf);
	return std::string(buf);
#else
	char buf[260] = { 0 };
	readlink("/proc/self/exe", buf, 260);
	return GetDirectory_(buf);
#endif
}

std::string PathUtil::GetDirectory_(const std::string& fileFullPath)
{
#ifdef _WIN32
	return std::string(fileFullPath, 0, fileFullPath.rfind("\\"));
#else
	assert(fileFullPath.size() > 1);
	if (fileFullPath.find('/', 1) == std::string::npos)
		return std::string("/");
	else
		return std::string(fileFullPath, 0, fileFullPath.rfind("/"));
#endif
}

std::string PathUtil::GetFile(const std::string& fileFullPath)
{
#ifdef _WIN32
	return std::string(fileFullPath, fileFullPath.rfind("\\") + 1);
#else
	return std::string(fileFullPath, fileFullPath.rfind("/") + 1);
#endif
}

std::string PathUtil::PathCombine_(const std::string& path1, const std::string& path2)
{
#ifdef _WIN32
	char buf[MAX_PATH] = { 0 };
	::PathCombineA(buf, path1.c_str(), path2.c_str());
	return std::string(buf);
#else
	assert(path2[0] != '/');
	std::string path_(path1);
	if (path_[path_.size() - 1] != '/')
		path_ += std::string("/");
	return path_ + path2;
#endif
}

bool PathUtil::exist(const std::string& path_)
{
#ifdef _WIN32
	return ::PathFileExistsA(path_.c_str()) ? true : false;
#else
	struct stat sts;
	if ((stat(path_.c_str(), &sts)) == -1 && errno == ENOENT)
		return false;
	return true;
#endif
}

bool PathUtil::IsStrJustFileName_(const std::string& str)
{
	return !(
		(str.find(':', 0) != std::string::npos) ||
		(str.find('\\', 0) != std::string::npos) ||
		(str.find('/', 0) != std::string::npos));
}

void PathUtil::CreateFolder_(const std::string& directory_)
{
	if (!PathUtil::exist(directory_))
	{
#ifdef _WIN32
		::SHCreateDirectoryExA(NULL, directory_.c_str(), NULL);
#else
		std::string s("mkdir -p " + directory_);
		system(s.c_str());
#endif
	}
}

