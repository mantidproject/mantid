#include "test_helper.h"
#include "MantidKernel/ConfigService.h"
#include <cstdarg>
#include <hdf5.h>
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

bool NexusTest::hdf_file_is_closed(std::string const &filename) {
  ssize_t file_count = H5Fget_obj_count(H5F_OBJ_ALL, H5F_OBJ_FILE);
  if (file_count < 0) {
    throw std::runtime_error("failure to get opened file count");
  } else if (file_count == 0) {
    // no files are opened
    return true;
  } else {
    // some files are opened -- see if maybe ours is
    std::vector<hid_t> file_ids(file_count);
    ssize_t ret = H5Fget_obj_ids(H5F_OBJ_ALL, H5F_OBJ_FILE, file_count, file_ids.data());
    if (ret < 0) {
      throw std::runtime_error("failure to find opened files");
    } else if (ret == 0) {
      return true;
    } else {
      for (hid_t file_id : file_ids) {
        // get the name and check it
        ssize_t name_size = H5Fget_name(file_id, nullptr, 0);
        std::string check_name(name_size, 'X');
        H5Fget_name(file_id, check_name.data(), name_size + 1);
        if (check_name == filename) {
          return false;
        }
      }
      return true;
    }
  }
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
