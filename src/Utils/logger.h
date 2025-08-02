#pragma once

#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

class Logger {
 public:
  static Logger& getInstance() {
    static Logger logger;
    return logger;
  }

  template <typename... Args>
  void log(const char* func, const int line, const char* fmt, Args&&... args) {
    constexpr size_t INTERNAL_FORMAT_SIZE = 512;
    char internal_format[INTERNAL_FORMAT_SIZE + 1]{0};
    std::snprintf(internal_format, INTERNAL_FORMAT_SIZE,
                  "%%s.%%03d|%%s:%%d|%s\n", fmt);

    const std::chrono::system_clock::time_point cpp_time =
        std::chrono::system_clock::now();
    const std::time_t c_time = std::chrono::system_clock::to_time_t(cpp_time);
    const int ms =
        static_cast<int>((std::chrono::duration_cast<std::chrono::milliseconds>(
                              cpp_time.time_since_epoch()) %
                          1000)
                             .count());
#if defined(_MSC_VER)
    std::tm c_tm;
    localtime_s(&c_tm, &c_time);
#else
    const std::tm c_tm = *std::localtime(&c_time);
#endif

    std::stringstream ss;
    ss << std::put_time(&c_tm, "%Y-%m-%dT%H:%M:%S");

    const size_t write_size =
        std::snprintf(buffer.data(), BUFFER_SIZE, internal_format,
                      ss.str().c_str(), ms, func, line, args...);

    std::ofstream of("logs.log", std::ios::app);
    if (of.is_open()) {
      of.write(buffer.data(), write_size);
      of.flush();
      of.close();
    }
  }

 private:
  static constexpr size_t BUFFER_SIZE = 64 * 1024;
  std::vector<char> buffer;

  Logger() : buffer(BUFFER_SIZE + 1, 0) {}
  ~Logger() {}
};

#if defined(__clang__) || defined(__GNUC__)
#define FUNC_SIGNATURE __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
#define FUNC_SIGNATURE __FUNCSIG__
#elif defined(__func__)
#define FUNC_SIGNATURE __func__
#else
#define FUNC_SIGNATURE "UnknownFunction"
#endif

#ifdef IS_DEBUG_BUILD
#define MYLOG(some_bigger_format_variable, ...)       \
  Logger::getInstance().log(FUNC_SIGNATURE, __LINE__, \
                            some_bigger_format_variable, __VA_ARGS__)
#else
#define MYLOG(some_bigger_format_variable, ...)
#endif
