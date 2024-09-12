// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidFrameworkTestHelpers/FileResource.h"
#include <filesystem>
#include <string>

FileResource::FileResource(const std::string &fileName, bool debugMode) : m_debugMode(debugMode) {

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
}

void FileResource::setDebugMode(bool mode) { m_debugMode = mode; }
std::string FileResource::fullPath() const { return m_full_path.generic_string(); }

FileResource::~FileResource() {

  // file is removed at end of file handle's lifetime
  if (std::filesystem::is_regular_file(m_full_path)) {
    if (!m_debugMode)
      std::filesystem::remove(m_full_path);
    else
      std::cout << "Debug file at: " << m_full_path << " not removed. " << std::endl;
  }
}
