#ifndef MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGDIALOG_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGDIALOG_H_

#include "MantidQtCustomInterfaces/Tomography/TomoPathsConfig.h"
#include "MantidQtCustomInterfaces/Tomography/TomoReconToolsUserSettings.h"

//------------------------------
// Forward Declarations
//------------------------------
namespace MantidQt {
namespace CustomInterfaces {
struct TomoReconToolsUserSettings;
}
}

namespace MantidQt {
namespace CustomInterfaces {
/**
Third party tool configuration dialog(s) for the tomographic reconstruction
GUI.

Copyright &copy; 2014,2015 ISIS Rutherford Appleton Laboratory, NScD
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
class TomoToolConfigDialogBase {
public:
  TomoToolConfigDialogBase(std::string toolMethod = "")
      : m_toolMethod(toolMethod), m_isInitialised(false) {}
  virtual ~TomoToolConfigDialogBase() {}

  static TomoToolConfigDialogBase *
  getCorrectDialogForToolFromString(const std::string &toolName);

  void setupDialog(std::string runPath, TomoPathsConfig paths,
                   std::string pathOut, std::string localOutNameAppendix) {

    if (!m_isInitialised) {
      setupPaths(runPath, paths, pathOut, localOutNameAppendix);
      setupDialogUi();
      m_isInitialised = true;
    }
  }

  /// Runs the dialogue and handles the returns
  virtual int execute();

  virtual bool isInitialised() { return m_isInitialised; }

  virtual TomoReconToolsUserSettings getReconToolSettings() {
    return m_toolSettings;
  }

  std::string getSelectedToolMethod() const { return m_toolMethod; }

protected:
  virtual void setScriptRunPath(std::string run) { m_runPath = run; }

  virtual void setTomoPathsConfig(TomoPathsConfig paths) { m_paths = paths; }

  virtual void setPathOut(std::string pathOut) { m_pathOut = pathOut; }

  virtual void setLocalOutNameAppendix(std::string localOutNameAppendix) {
    m_localOutNameAppendix = localOutNameAppendix;
  }

  virtual void setupPaths(std::string runPath, TomoPathsConfig paths,
                          std::string pathOut,
                          std::string localOutNameAppendix) {
    this->setScriptRunPath(runPath);
    this->setTomoPathsConfig(paths);
    this->setPathOut(pathOut);
    this->setLocalOutNameAppendix(localOutNameAppendix);
  }

  virtual void setupDialogUi() = 0;

  // setup the tool config with the correct paths
  // save the chosen method ?
  virtual void setupToolConfig() = 0;

  virtual void handleDialogResult(int result);

  virtual int executeQt() = 0;

  TomoReconToolsUserSettings m_toolSettings;

  std::string m_toolMethod;

  std::string m_runPath;

  std::string m_localOutNameAppendix;

  std::string m_pathOut;

  TomoPathsConfig m_paths;

  bool m_isInitialised;
};
}
}

#endif // MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGDIALOG_H_
