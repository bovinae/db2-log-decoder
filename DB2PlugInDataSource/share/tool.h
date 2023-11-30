#pragma once
#include <string>
#include <cstdint>
#include <vector>

namespace tool
{
    bool is_big_endian();

    void split(std::vector<std::string>& out, const std::string& in, const std::string& delims = ",");

    void replace_chars(std::string& str, const std::vector<char>& to_replace, const char replace_with);

    // trim from start (in place)
    void ltrim(std::string& s);

    // trim from end (in place)
    void rtrim(std::string& s);

    // trim from both ends (in place)
    void trim(std::string& s);

    // trim from start (copying)
    std::string ltrim_copy(std::string s);

    // trim from end (copying)
    std::string rtrim_copy(std::string s);

    // trim from both ends (copying)
    std::string trim_copy(std::string s);

    std::string get_app_path();
    std::string get_exe_path();
    std::string get_process_name();
    std::string get_thread_id();
    uint32_t get_rand_id();
    std::string random_string(std::size_t length);
    bool file_exist(const char* path);
    bool read_whole_file(const char* path, std::vector<char>& content);
    bool write_whole_file(const char* path, const std::vector<char>& content);
    bool write_whole_file(const char* path, const char* buff, size_t size);
    bool reomve_file(const char* path);
    bool create_directories(const char* path);
    bool get_files_under_directory(const char* path, std::vector<std::string>& paths, const char* suffix);
    bool get_files_under_directory(const char* path, std::vector<std::pair<std::string, std::string>>& name_paths, const char* suffix);
    uint64_t get_utc_msecs_since_epoch(int timezone = 8) noexcept;
}