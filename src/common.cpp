#include "common.h"
#include <string>
#include "CppProgressBar.h"


CppProgressBar cpb;

simtime_picosec pkt_transfer_time(uint64_t pkt_size) {
    return pkt_size/*num elements*/* 320; // 100Gbps
}

int myprintf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    auto size = std::vsnprintf(nullptr, 0, format, args);
    std::string s(size + 1, '\0');
    va_start(args, format);
    auto r = std::vsprintf(&s[0], format, args);
    cpb.update_variable();
    cpb.stdout_in_for_progress(s);
    va_end(args);
    return r;
}