#pragma once
#include <algorithm>

namespace tool
{

    void init_remote_big_endian(bool big_endian);

    bool need_reverse();

    //字节翻转
    template<typename T>
    void reverse_bytes(T* start)
    {
        if (!need_reverse() || !*start)
            return;
        char* istart = (char*)start, * iend = istart + sizeof(T);
        std::reverse(istart, iend);
    }

    //值翻转
    template<typename T>
    T reverse_value(T val)
    {
        reverse_bytes(&val);
        return val;
    }
}