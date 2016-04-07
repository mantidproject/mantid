#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_STACKOFIMAGESDIR_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_STACKOFIMAGESDIR_H_

#include "MantidKernel/System.h"

#include <string>
#include <vector>

// forward declarations
namespace Poco {
class File;
}

namespace MantidQt {
namespace CustomInterfaces {

/**
Represents the structure of directories where a stack of images is
stored on disk: data/flat/dark + processed + pre_filtered, etc.

Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD
Oak Ridge National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport StackOfImagesDirs {
public:
  /// constructor from a path (to a directory)
  StackOfImagesDirs(const std::string &path, bool allowSimpleLayout = true);

  /// The current (simple) concept of valid is: there at least a
  /// sample/data directory with at least one file in it.
  bool isValid() const { return m_valid; }

  // human readable description of the expected structure of directories
  std::string description() const;

  // string that describes the status/error message about this directory
  std::string status() const;

  std::string sampleImagesDir() const { return m_sampleDir; };
  std::string flatImagesDir() const { return m_flatDir; };
  std::string darkImagesDir() const { return m_darkDir; };

  std::vector<std::string> sampleFiles() const;
  std::vector<std::string> flatFiles() const;
  std::vector<std::string> darkFiles() const;

private:
  /// tries to find the expected data_/dark_/flat_ directories
  void findStackDirs(const std::string &path, bool allowSimpleLayout);

  /// simple version: just files (sample data)
  bool findStackFilesSimple(Poco::File &dir);

  /// complex version: sample (data), flat, and dark
  bool findStackDirsComplex(Poco::File &dir);

  /// finds images in a directory
  std::vector<std::string> findImgFiles(const std::string &path) const;

  /// passes basic validity checks
  bool m_valid;
  /// string with informative messages specially when not valid
  std::string m_statusDescStr;

  std::string m_sampleDir;
  std::string m_flatDir;
  std::string m_darkDir;

  std::string m_descr;

  static const std::string g_descrComplex;
  static const std::string g_descrSimple;
  static const std::string g_descrBoth;
  static const std::string g_sampleNamePrefix;
  static const std::string g_flatNamePrefix;
  static const std::string g_darkNamePrefix;
  static const std::string g_processedNamePrefix;
  static const std::string g_prefilteredNamePrefix;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_STACKOFIMAGESDIR_H_
