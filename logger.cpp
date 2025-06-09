#include "webserv.hpp"

// ANSI color codes
const std::string RESET   = "\033[0m";
const std::string RED     = "\033[31m";
const std::string GREEN   = "\033[32m";
const std::string YELLOW  = "\033[33m";
const std::string BLUE    = "\033[34m";
const std::string MAGENTA = "\033[35m";
const std::string CYAN    = "\033[36m";
const std::string WHITE   = "\033[37m";
const std::string GRAY    = "\033[90m";

std::string get_level_color(LogLevel level) {
    switch (level) {
        case INFO:    return GREEN;
        case WARNING: return YELLOW;
        case ERROR:   return RED;
        case DEBUG:   return BLUE;
        default:      return RESET;
    }
}
std::string current_time() {
  time_t now = time(0);
  char buf[80];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
  return buf;
}

std::string level_to_string(LogLevel level) {
  switch (level) {
    case INFO: return "INFO";
    case WARNING: return "WARNING";
    case ERROR: return "ERROR";
    case DEBUG: return "DEBUG";
    default: return "UNKNOWN";
  }
}

void log_message(LogLevel level, const std::string& msg, const char* file, int line) {
  std::string time_str  = current_time();
  std::string level_str = level_to_string(level);
  std::string color     = get_level_color(level);

  // Colored timestamp
  std::cout << "[" << CYAN << time_str << RESET << "] ";

  // Colored log level
  std::cout << "[" << color << level_str << RESET << "] ";

  // Colored file:line (only if not INFO)
  if (level != INFO) {
      std::cout << MAGENTA << file << ":" << line << RESET << " - ";
  }

  // Message
  std::cout << msg << std::endl;
}
