// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

/*
 * RAII: Gives a clean file destination and removes the file when
 * handle is out of scope. Must be stack allocated.
 *
 * @author Takudzwa Makoni, RAL (UKRI), ISIS
 * @date 06/08/2019
 */

#ifndef MANTID_NEXUSGEOMETRY_FILERESOURCE_H_
#define MANTID_NEXUSGEOMETRY_FILERESOURCE_H_

#include <boost/filesystem.hpp>
#include <iostream>
#include <string>

class ScopedFileHandle {

public:
  ScopedFileHandle(const std::string &fileName, bool debugMode = false)
      : m_debugMode(debugMode) {

    const auto temp_dir = boost::filesystem::temp_directory_path();
    auto temp_full_path = temp_dir;
    // append full path to temp directory to user input file name
    temp_full_path /= fileName;

    // Check proposed location and throw std::invalid argument if file does
    // not exist. otherwise set m_full_path to location.

    if (boost::filesystem::is_directory(temp_dir)) {
      m_full_path = temp_full_path;

    } else {
      throw std::invalid_argument("failed to load temp directory: " +
                                  temp_dir.generic_string());
    }
  }

  void setDebugMode(bool mode) { m_debugMode = mode; }
  std::string fullPath() const { return m_full_path.generic_string(); }

  ~ScopedFileHandle() {

    // file is removed at end of file handle's lifetime
    if (boost::filesystem::is_regular_file(m_full_path)) {
      if (!m_debugMode)
        boost::filesystem::remove(m_full_path);
      else
        std::cout << "Debug file at: " << m_full_path << " not removed. "
                  << std::endl;
    }
  }

private:
  bool m_debugMode;
  boost::filesystem::path m_full_path; // full path to file

  // prevent heap allocation for ScopedFileHandle
protected:
  static void *operator new(std::size_t); // prevent heap allocation of scalar.
  static void *operator new[](std::size_t); // prevent heap allocation of array.
};

#endif /* MANTID_NEXUSGEOMETRYY_FILERESOURCE_H_ */
