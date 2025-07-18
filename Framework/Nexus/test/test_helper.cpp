#include "test_helper.h"
#include "MantidKernel/ConfigService.h"
#include <cstdarg>
#include <iostream>

void NexusTest::removeFile(const std::string &filename) {
  if (std::filesystem::exists(filename)) {
    std::filesystem::remove(filename);
  }
}

std::string NexusTest::getFullPath(const std::string &filename) {
  using Mantid::Kernel::ConfigService;
  auto dataPaths = ConfigService::Instance().getDataSearchDirs();
  for (auto &dataPath : dataPaths) {
    const auto hdf5Path = std::filesystem::path(dataPath) / filename;
    if (std::filesystem::exists(hdf5Path)) {
      return hdf5Path.string();
    }
  }
  return std::string();
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

NexusTest::FileResource::FileResource(const std::string &fileName, bool debugMode) : m_debugMode(debugMode) {

  const auto temp_dir = std::filesystem::temp_directory_path();
  auto temp_full_path = temp_dir;
  // append full path to temp directory to user input file name
  temp_full_path /= fileName;

  // Check proposed location and throw std::invalid argument if file does
  // not exist. otherwise set m_full_path to location.

  if (std::filesystem::is_directory(temp_dir)) {
    m_full_path = temp_full_path;
  } else {
    throw std::invalid_argument("failed to load temp directory: " + temp_dir.generic_string());
  }

  // if the file already exists, remove it
  if (std::filesystem::is_regular_file(m_full_path)) {
    std::filesystem::remove(m_full_path);
  }
}

void NexusTest::FileResource::setDebugMode(bool mode) { m_debugMode = mode; }
std::string NexusTest::FileResource::fullPath() const { return m_full_path.generic_string(); }

NexusTest::FileResource::~FileResource() {

  // file is removed at end of file handle's lifetime
  if (std::filesystem::is_regular_file(m_full_path)) {
    if (!m_debugMode)
      std::filesystem::remove(m_full_path);
    else
      std::cout << "Debug file at: " << m_full_path << " not removed. " << std::endl;
  }
}
