#ifndef MANTID_CUSTOMINTERFACES_REFLSAVETABPRESENTER_H
#define MANTID_CUSTOMINTERFACES_REFLSAVETABPRESENTER_H

#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflSaveTabPresenter.h"
#include "MantidKernel/Logger.h"

namespace MantidQt {
namespace CustomInterfaces {

// Forward decs
class IReflSaveTabView;

/** @class ReflSaveTabPresenter

ReflSaveTabPresenter is a presenter class for the tab 'Save ASCII' in the
Reflectometry (Polref) Interface.

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
  void notify(IReflSaveTabPresenter::Flag flag) override;

  /// Adds all workspace names to the list of workspaces
  void populateWorkspaceList();
  /// Adds all workspace params to the list of logged parameters
  void populateParametersList(std::string wsName);
  /// Filter workspaces names
  void filterWorkspaceNames(std::string filter, bool regexCheck);
  /// Suggest a save directory
  void suggestSaveDir();
  /// Save selected workspaces
  void saveWorkspaces();

private:
  /// Obtains all available workspace names
  std::vector<std::string> getAvailableWorkspaceNames();

  /// The view
  IReflSaveTabView *m_view;
  /// Names of possible save algorithms
  std::vector<std::string> saveAlgs;
  /// Extensions used for each save algorithm
  std::vector<std::string> saveExts;

  static Mantid::Kernel::Logger g_log;
};
}
}
#endif /* MANTID_CUSTOMINTERFACES_REFLSAVETABPRESENTER_H */
