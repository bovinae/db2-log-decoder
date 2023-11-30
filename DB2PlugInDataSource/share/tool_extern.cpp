#include "tool_extern.h"
#include "tool.h"

static bool remote_big_endian = false;

void tool::init_remote_big_endian(bool big_endian)
{
    remote_big_endian = big_endian;
}

bool tool::need_reverse()
{
    return is_big_endian() != remote_big_endian;
}
