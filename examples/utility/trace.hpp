#pragma once

#include <source_location>

#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

auto dbg = [](const char* file, const char* func, std::uint32_t line) {
    std::cerr << "tid: " << std::this_thread::get_id() << " " << file << ":"
              << func << ":" << line << " called.\n";
};

#define DBG                                                                    \
    dbg(std::source_location::current().file_name(),                           \
        std::source_location::current().function_name(),                       \
        std::source_location::current().line())
