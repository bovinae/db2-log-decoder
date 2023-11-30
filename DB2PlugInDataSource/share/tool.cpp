#include <string>
#include <random>
#include <thread>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <chrono>
#include <algorithm>
#include "tool.h"

#ifdef _MSC_VER
#include <Windows.h>
#include <atlconv.h>
#else
#include <limits.h>
#include <unistd.h>
#endif
using namespace std;

namespace
{
	inline size_t check_path(const char* path)
	{
		if (!path)
			return 0;
		auto length = strlen(path);

#ifdef _MSC_VER
		if (length > MAX_PATH)
#else
		if (length > PATH_MAX)
#endif
			return 0;
		return length;
	}
}


static bool local_big_endian()
{
	union tt
	{
		int n;
		char a;
	};
	tt t;
	t.n = 1;
	return t.a != 1;
}


bool tool::is_big_endian()
{
	static const bool big_endian = local_big_endian();
	return big_endian;
}

void tool::split(vector<string>& out, const string& in, const string& delims)
{
	string::size_type start = in.find_first_not_of(delims);
	if (start == string::npos) return;

	string::size_type end = in.find_first_of(delims, start);
	while (end != string::npos) {
		out.push_back(in.substr(start, end - start));
		start = in.find_first_not_of(delims, end);
		if (start == string::npos) {
			return;
		}

		end = in.find_first_of(delims, start);
	}
	out.push_back(in.substr(start));
}

void tool::replace_chars(string& str, const vector<char>& to_replace, const char replace_with)
{
	for (auto old : to_replace)
		replace(str.begin(), str.end(), old, replace_with);
}

void tool::ltrim(string& s)
{
	s.erase(s.begin(), find_if(s.begin(), s.end(), [](unsigned char ch) {
		return !isspace(ch);
		}));
}

void tool::rtrim(string& s)
{
	s.erase(find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
		return !isspace(ch);
		}).base(), s.end());
}

void tool::trim(string& s)
{
	ltrim(s);
	rtrim(s);
}

string tool::ltrim_copy(string s)
{
	ltrim(s);
	return s;
}

string tool::rtrim_copy(string s)
{
	rtrim(s);
	return s;
}

string tool::trim_copy(string s)
{
	trim(s);
	return s;
}

string tool::get_app_path()
{
	string path;
#ifdef _MSC_VER
	TCHAR szFullPath[260] = { 0 };
	::GetModuleFileName(nullptr, szFullPath, MAX_PATH);

	USES_CONVERSION;
	path = W2A(szFullPath);
#else
	char current_path[PATH_MAX]{};
	int cnt = readlink("/proc/self/exe", current_path, PATH_MAX);
	if (cnt < 0 || cnt >= PATH_MAX) {
		return path;
	}
	path = current_path;
#endif
	return path;
}

string tool::get_exe_path()
{
	string path;
#ifdef _MSC_VER
	TCHAR szFullPath[260] = { 0 };
	::GetModuleFileName(nullptr, szFullPath, MAX_PATH);

	USES_CONVERSION;
	path = W2A(szFullPath);
	path = path.substr(0, path.find_last_of("\\"));
#else
	char current_path[PATH_MAX]{};
	int cnt = readlink("/proc/self/exe", current_path, PATH_MAX);
	if (cnt < 0 || cnt >= PATH_MAX) {
		return path;
	}
	for (int i = cnt; i >= 0; --i) {
		if (current_path[i] == '/') {
			current_path[i + 1] = '\0';
			break;
		}
	}
	path = current_path;
#endif
	return path;
}

string tool::get_process_name()
{
	string result;

#ifdef _MSC_VER
	TCHAR szFullPath[260] = { 0 };
	::GetModuleFileName(nullptr, szFullPath, MAX_PATH);

	USES_CONVERSION;
	result = W2A(szFullPath);
	size_t last_slash_pos = result.rfind('/');
	if (last_slash_pos != string::npos) {
		result = result.substr(last_slash_pos + 1);
	}
#else

	char current_path[PATH_MAX]{};
	int cnt = readlink("/proc/self/exe", current_path, PATH_MAX);
	if (cnt < 0 || cnt >= PATH_MAX) {
		return result;
	}

	result.assign(current_path, cnt);
	size_t last_slash_pos = result.rfind('/');
	if (last_slash_pos != string::npos) {
		result = result.substr(last_slash_pos + 1);
	}
#endif
	return result;
}

string tool::get_thread_id()
{
	stringstream sin;
	sin << this_thread::get_id();
	return sin.str();
}

uint32_t tool::get_rand_id()
{
	random_device r;
	uniform_int_distribution<unsigned int> u(0, 0xffffffff);
	default_random_engine e(r());
	return u(e);
}

string tool::random_string(size_t length)
{
	const static string CHARACTERS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	mt19937 generator(time(nullptr) ^ random_device()());
	uniform_int_distribution<> distribution(0, CHARACTERS.size() - 1);

	string random_string;
	random_string.reserve(length);
	for (size_t i = 0; i < length; ++i)
		random_string += CHARACTERS[distribution(generator)];

	return random_string;
}

bool tool::file_exist(const char* path)
{
	try
	{
		return filesystem::exists(path);
	}
	catch (...)
	{
		return false;
	}
}

bool tool::read_whole_file(const char* path, vector<char>& content)
{
	if (!file_exist(path))
		return false;
	ifstream in{ path, ios::binary | ios::in };
	do
	{
		if (!in)
			break;
		if (!in.seekg(0, in.end))
			break;
		auto length = in.tellg();
		if (!in.seekg(0, in.beg))
			break;
		content.resize(static_cast<int>(length));
		return in.read(content.data(), length).operator bool();
	} while (false);
	return false;
}

bool tool::reomve_file(const char* path)
{
	try
	{
		return filesystem::remove_all(path);
	}
	catch (...)
	{
		return false;
	}
}

bool tool::create_directories(const char* path)
{
	try
	{
		if (filesystem::is_directory(path))
			return true;
		return filesystem::create_directories(path);
	}
	catch (...)
	{
		return false;
	}
}

bool tool::write_whole_file(const char* path, const vector<char>& content)
{
	if (!check_path(path))
		return false;
	ofstream out{ path, ios::binary | ios::out };
	do
	{
		if (!out)
			break;
		return out.write(content.data(), content.size()).operator bool();
	} while (false);
	return false;
}

bool tool::write_whole_file(const char* path, const char* buff, size_t size)
{
	if (!buff)
		return false;
	if (!check_path(path))
		return false;
	ofstream out{ path, ios::binary | ios::out };
	do
	{
		if (!out)
			break;
		return out.write(buff, size).operator bool();
	} while (false);
	return false;
}

bool tool::get_files_under_directory(const char* path, vector<string>& paths, const char* suffix)
{
	try
	{
		filesystem::path fullpath(path);
		filesystem::recursive_directory_iterator end_iter;
		for (filesystem::recursive_directory_iterator iter(fullpath); iter != end_iter; iter++)
		{
			try
			{
				if (!filesystem::is_directory(*iter) && !filesystem::is_symlink(*iter))
				{
					if (iter->path().extension() != suffix)
						continue;
					paths.emplace_back(iter->path().string());
				}

			}
			catch (const exception& ex)
			{
				continue;
			}
		}
	}
	catch (...)
	{
		return false;
	}
	return true;
}

bool tool::get_files_under_directory(const char* path, vector<pair<string, string>>& name_paths, const char* suffix)
{
	try
	{
		filesystem::path fullpath(path);
		filesystem::recursive_directory_iterator end_iter;
		for (filesystem::recursive_directory_iterator iter(fullpath); iter != end_iter; iter++)
		{
			try
			{
				if (!filesystem::is_directory(*iter) && !filesystem::is_symlink(*iter))
				{
					if (iter->path().extension() != suffix)
						continue;
					string file = iter->path().string();
					string name = iter->path().stem().string();
					name_paths.emplace_back(move(name), move(file));
				}
			}
			catch (const exception& ex)
			{
				continue;
			}
		}
	}
	catch (...)
	{
		return false;
	}
	return true;
}

uint64_t tool::get_utc_msecs_since_epoch(int timezone) noexcept
{
	using namespace chrono;
	return  duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() + timezone * 3600 * 1000;
}
