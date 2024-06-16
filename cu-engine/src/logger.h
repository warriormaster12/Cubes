#pragma once
#include <fmt/core.h>
#include <fmt/color.h>
#include <fmt/chrono.h>

#define ENGINE_INFO(...) ::fmt::print(fg(fmt::color::lawn_green), "[{:%H:%M}]: {}\n", std::chrono::system_clock::now(), fmt::format(__VA_ARGS__))
#define ENGINE_WARN(...) ::fmt::print(fg(fmt::color::yellow), "[{:%H:%M}]: {}\n", std::chrono::system_clock::now(), fmt::format(__VA_ARGS__))
#define ENGINE_ERROR(...) ::fmt::print(fg(fmt::color::crimson), "[{:%H:%M}]: {}\n", std::chrono::system_clock::now(), fmt::format(__VA_ARGS__))
