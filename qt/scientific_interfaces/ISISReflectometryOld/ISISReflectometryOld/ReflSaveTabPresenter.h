#ifndef MANTID_CUSTOMINTERFACES_REFLSAVETABPRESENTER_H
#define MANTID_CUSTOMINTERFACES_REFLSAVETABPRESENTER_H

#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflSaveTabPresenter.h"
#include <vector>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

// Forward decs
class IReflMainWindowPresenter;
class IReflSaveTabView;

/** @class ReflSaveTabPresenter

ReflSaveTabPresenter is a presenter class for the tab 'Save ASCII' in the
ISIS Reflectometry Interface.

Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

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

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTIDQT_CUSTOMINTERFACES_DLL ReflSaveTabPresenter
    : public IReflSaveTabPresenter {
public:
  /// Constructor
  ReflSaveTabPresenter(IReflSaveTabView *view);
  /// Destructor
  ~ReflSaveTabPresenter() override;
  /// Accept a main presenter
  void acceptMainPresenter(IReflMainWindowPresenter *mainPresenter) override;
  void notify(IReflSaveTabPresenter::Flag flag) override;

private:
  /// Adds all workspace names to the list of workspaces
  void populateWorkspaceList();
  /// Adds all workspace params to the list of logged parameters
  void populateParametersList();
  /// Filter workspaces names
  void filterWorkspaceNames();
  /// Suggest a save directory
  void suggestSaveDir();
  /// Save selected workspaces to a directory
  void saveWorkspaces();
  /// Obtains all available workspace names
  std::vector<std::string> getAvailableWorkspaceNames();

  /// The view
  IReflSaveTabView *m_view;
  /// The main presenter
  IReflMainWindowPresenter *m_mainPresenter;
  /// Names of possible save algorithms
  std::vector<std::string> m_saveAlgs;
  /// Extensions used for each save algorithm
  std::vector<std::string> m_saveExts;
};
}
}
#endif /* MANTID_CUSTOMINTERFACES_REFLSAVETABPRESENTER_H */
