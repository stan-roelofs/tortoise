#ifndef TORTOISE_LOG_HPP
#define TORTOISE_LOG_HPP

#include <cinttypes>
#include <iostream>

#ifdef TORTOISE_ENABLE_LOGGING
#define LOG(tag, s, ...) printf(tag " " s "\n", ##__VA_ARGS__)
#else
#define LOG(...) ((void)0)
#endif

#endif