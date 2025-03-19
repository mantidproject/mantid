#include "test_helper.h"
#include <cstdarg>
#include <filesystem>

std::string NexusTest::getFullPath(std::string const &filename) {
  std::filesystem::path there(filename);
  if (there.is_relative()) {
    there = "";
    std::filesystem::path here = std::filesystem::current_path();
    auto p = here.begin();
    for (; p != here.end(); p++) {
      there /= *p;
      if (*p == "mantid") {
        break;
      }
    }
    if (p == here.end()) {
      throw std::runtime_error("");
    }
    there /= "build/ExternalData/Testing/Data/UnitTest/";
    there /= filename;
  }
  return there;
}

void NexusTest::removeFile(const std::string &filename) {
  if (std::filesystem::exists(filename)) {
    std::filesystem::remove(filename);
  }
}

/**
 * Let's face it, std::string is poorly designed,
 * and this is the constructor that it needed to have.
 * Initialize a string from a c-style formatting string.
 */
std::string NexusTest::strmakef(const char *const fmt, ...) {
  char buf[256];

  va_list args;
  va_start(args, fmt);
  const auto r = std::vsnprintf(buf, sizeof buf, fmt, args);
  va_end(args);

  if (r < 0)
    // conversion failed
    return {};

  const size_t len = r;
  if (len < sizeof buf)
    // we fit in the buffer
    return {buf, len};

  std::string s(len, '\0');
  va_start(args, fmt);
  std::vsnprintf(&(*s.begin()), len + 1, fmt, args);
  va_end(args);
  return s;
}
