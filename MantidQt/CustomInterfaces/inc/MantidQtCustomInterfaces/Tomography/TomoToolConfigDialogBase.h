#ifndef MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGDIALOGBASE_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGDIALOGBASE_H_

#include "MantidQtCustomInterfaces/Tomography/TomoPathsConfig.h"
#include "MantidQtCustomInterfaces/Tomography/TomoReconToolsUserSettings.h"

#include <memory>

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
  TomoToolConfigDialogBase(const std::string toolName = "",
                           const std::string toolMethod = "")
      : m_toolName(toolName), m_toolMethod(toolMethod), m_isInitialised(false) {
  }
  virtual ~TomoToolConfigDialogBase() {}

  /// public static function accessor to create dialogues
  static std::unique_ptr<TomoToolConfigDialogBase>
  getToolDialogFor(const std::string &toolName);

  /// Sets up the dialogue settings, but does not initialise a QDialog
  void setupDialog(const std::string &runPath, const TomoPathsConfig &paths,
                   const std::string &pathOut,
                   const std::string &localOutNameAppendix) {

    setupPaths(runPath, paths, pathOut, localOutNameAppendix);
    setupToolSettingsFromPaths();
  }

  /// initialises a QDialog and handles the returns
  virtual int initialiseGUIandExecute() {
    if (!isInitialised()) {
      // set up the tool's method on the first run
      // this prevents from creating and destroying many dialogues if the user
      // decides to scroll quickly, and the dialogue is only initialised if the
      // user clicks the "Setup" button. If the tool is not setup the default
      // settings will be provided if the user clicks Reconstruct
      initialiseDialog();
      setupDialogUi();
      setupMethodSelected();

      m_isInitialised = true;
    }

    const int res = this->executeQt();
    this->handleDialogResult(res);
    return res;
  }

  virtual bool isInitialised() const { return m_isInitialised; }

  std::string getSelectedToolMethod() const { return m_toolMethod; }

  /// return pointer and transfer ownership
  std::shared_ptr<TomoRecToolConfig> getSelectedToolSettings() const {
    return m_toolSettings;
  }

  std::string getSelectedToolName() const { return m_toolName; }

protected:
  virtual void initialiseDialog() = 0;

  virtual void handleDialogResult(const int result);

  virtual void setScriptRunPath(const std::string run) { m_runPath = run; }

  virtual void setTomoPathsConfig(const TomoPathsConfig paths) {
    m_paths = paths;
  }

  virtual void setPathOut(const std::string pathOut) { m_pathOut = pathOut; }

  virtual void setLocalOutNameAppendix(const std::string localOutNameAppendix) {
    m_localOutNameAppendix = localOutNameAppendix;
  }

  virtual void setupPaths(const std::string &runPath,
                          const TomoPathsConfig &paths,
                          const std::string &pathOut,
                          const std::string &localOutNameAppendix) {
    this->setScriptRunPath(runPath);
    this->setTomoPathsConfig(paths);
    this->setPathOut(pathOut);
    this->setLocalOutNameAppendix(localOutNameAppendix);
  }

  virtual void setupDialogUi() = 0;

  /// setup the selected method member variable
  virtual void setupMethodSelected() = 0;

  /// setup the tool config with the correct paths, must be called after the
  /// paths have been set!
  virtual void setupToolSettingsFromPaths() = 0;

  /// provided virtual function to add Qt execute behaviour as necessary
  virtual int executeQt() = 0; // this class doesn't inherit from Qt and doesnt
                               // have this->exec()

  // empty function body as not all tools have methods
  virtual std::vector<std::pair<std::string, std::string>> getToolMethods() {
    return {};
  }

  std::shared_ptr<TomoRecToolConfig> m_toolSettings;

  const std::string m_toolName;

  std::string m_toolMethod;

  std::string m_runPath;

  std::string m_localOutNameAppendix;

  std::string m_pathOut;

  TomoPathsConfig m_paths;

  bool m_isInitialised;
};
}
}

#endif // MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGDIALOGBASE_H_
