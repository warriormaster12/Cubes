#pragma once
#include <fmt/core.h>
#include <fmt/color.h>

#define ENGINE_INFO(...) ::fmt::print(fg(fmt::color::lawn_green), fmt::format(__VA_ARGS__) + "\n");
#define ENGINE_WARN(...) ::fmt::print(fg(fmt::color::yellow), fmt::format(__VA_ARGS__) + "\n");
#define ENGINE_ERROR(...) ::fmt::print(fg(fmt::color::crimson), fmt::format(__VA_ARGS__) + "\n");