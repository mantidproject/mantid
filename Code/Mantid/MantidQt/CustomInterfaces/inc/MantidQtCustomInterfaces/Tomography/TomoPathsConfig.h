#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOPATHSCONFIG_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOPATHSCONFIG_H_

#include "MantidKernel/System.h"
#include "MantidQtCustomInterfaces/Tomography/ToolConfigAstraToolbox.h"
#include "MantidQtCustomInterfaces/Tomography/ToolConfigCustom.h"
#include "MantidQtCustomInterfaces/Tomography/ToolConfigTomoPy.h"

#include <string>

namespace MantidQt {
namespace CustomInterfaces {

/**
Represents one particular configuration of paths for tomography
jobs. These paths point to sample, open bean, flat etc. data (images)
that define where the data for a particular dataset can be found, and
that are normally required to run reconstruction tools but also pre-
and post- processing steps.

Copyright &copy; 2014,205 ISIS Rutherford Appleton Laboratory, NScD
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
class DLLExport TomoPathsConfig {
public:
  /**
   * Default constructor, makes a paths configuration which probably
   * needs additional input from the user before being usable (see
   * validate()).
   */
  TomoPathsConfig();

  /**
   * If the result of this validation is true, that must be
   * interpreted as: "this is ready to be used to run reconstruction
   * or other types of tools." Answers the question: is it non-empty
   * and apparently correct and usable? This will be false if it is
   * not initialized or any inconsistency can be detected.
   *
   * @return whether this configuration seems to be ready to be used.
   */
  bool validate() const;

  /**
   * Path to the sample files (sample images, typically FITS files)
   *
   * @return path (full or relative) as a string.
   */
  std::string pathSamples() const { return m_pathFITS; }

  void updatePathSamples(const std::string &p) { m_pathFITS = p; }

  /**
   * Path to the open beam file(s) (sample images, typically FITS files)
   * Note: open bean a.k.a. 'flat' or 'white'
   *
   * @return path (full or relative, directory or file) as a string.
   */
  std::string pathOpenBeam() const { return m_pathFlat; }

  void updatePathOpenBeam(const std::string &p) { m_pathFlat = p; }

  /**
   * Path to the dark image file(s)
   *
   * @return path (full or relative, directory or file) as a string.
   */
  std::string pathDark() const { return m_pathDark; }

  void updatePathDark(const std::string &p) { m_pathDark = p; }

  /**
   * Path to a base directory (for data usually). All other paths
   * could be absolute or relative to this one.
   *
   * @return path (full, directory) as a string.
   */
  std::string pathBase() const { return m_pathBase; }

  void updatePathBase(const std::string &p) { m_pathBase = p; }

  std::string pathSavuConfig() const;

  std::string pathScriptsTools() const { return m_pathScriptsTools; }

  void updatePathScriptsTools(const std::string &p) { m_pathScriptsTools = p; }

private:
  /// file paths, base dir on scarf
  std::string m_pathBase;
  /// path to fits file (sample data)
  std::string m_pathFITS;
  /// path to flat/open beam/bright image
  std::string m_pathFlat;
  /// path to dark image
  std::string m_pathDark;

  /// path to scripts/binaries
  std::string m_pathScriptsTools;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOPATHSCONFIG_H_
