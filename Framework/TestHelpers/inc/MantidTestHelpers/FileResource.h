// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

/** RAII: Gives a clean file destination and removes the file when
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

class FileResource {

public:
  FileResource(const std::string &fileName, bool debugMode = false);
  void setDebugMode(bool mode);
  std::string fullPath() const;
  ~FileResource();

private:
  bool m_debugMode;
  boost::filesystem::path m_full_path; // full path to file
  // prevent heap allocation for ScopedFileHandle
protected:
  static void *operator new(std::size_t); // prevent heap allocation of scalar.
  static void *operator new[](std::size_t); // prevent heap allocation of array.
};

#endif /* MANTID_NEXUSGEOMETRYY_FILERESOURCE_H_ */
