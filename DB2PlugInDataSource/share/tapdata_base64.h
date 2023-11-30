#pragma once
#include <string>
#include <vector>

namespace tool
{
	std::string base64_encode(const char * bytes_to_encode, unsigned int in_len);
	std::string base64_decode(const std::string  & encoded_string);
	std::vector<char> base64_decode(const char* data, size_t data_size);
}
