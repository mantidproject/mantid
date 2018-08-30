#ifndef MANTID_CUSTOMINTERFACES_REFLSAVETABPRESENTER_H
#define MANTID_CUSTOMINTERFACES_REFLSAVETABPRESENTER_H

#include "DllConfig.h"
#include "IReflAsciiSaver.h"
#include "IReflSaveTabPresenter.h"
#include "IReflSaveTabView.h"
#include <vector>
#include <string>
#include <memory>
#include <MantidKernel/ConfigPropertyObserver.h>
#include <boost/optional.hpp>
#include "IReflAsciiSaver.h"

namespace MantidQt {
namespace CustomInterfaces {

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
class MANTIDQT_ISISREFLECTOMETRY_DLL ReflSaveTabPresenter
    : public IReflSaveTabPresenter {
public:
  ReflSaveTabPresenter(IReflSaveTabView* view,
                       std::unique_ptr<IReflAsciiSaver> saver);
  /// Accept a main presenter
  void acceptMainPresenter(IReflBatchPresenter *mainPresenter) override;
  void notify(IReflSaveTabPresenter::Flag flag) override;
  void completedGroupReductionSuccessfully(
      MantidWidgets::DataProcessor::GroupData const &group,
      std::string const &workspaceName) override;
  void completedRowReductionSuccessfully(
      MantidWidgets::DataProcessor::GroupData const &group,
      std::string const &workspaceName) override;
  void onAnyReductionPaused() override;
  void onAnyReductionResumed() override;

private:
  bool isValidSaveDirectory(std::string const &directory);
  void onSavePathChanged();
  void warnInvalidSaveDirectory();
  void errorInvalidSaveDirectory();
  void warn(std::string const &message, std::string const &title);
  void error(std::string const &message, std::string const &title);
  /// Adds all workspace names to the list of workspaces
  void populateWorkspaceList();
  /// Adds all workspace params to the list of logged parameters
  void populateParametersList();
  /// Filter workspaces names
  void filterWorkspaceNames();
  /// Suggest a save directory
  void suggestSaveDir();
  /// Save selected workspaces to a directory
  void saveSelectedWorkspaces();
  /// Save specified workspaces to a directory
  void saveWorkspaces(std::vector<std::string> const &workspaceNames);
  void saveWorkspaces(std::vector<std::string> const &workspaceNames,
                      std::vector<std::string> const &logParameters);
  /// Obtains all available workspace names
  std::vector<std::string> getAvailableWorkspaceNames();
  NamedFormat formatFromIndex(int formatIndex) const;
  FileFormatOptions getSaveParametersFromView() const;
  void enableAutosave();
  void disableAutosave();
  bool shouldAutosave() const;

  /// The view
  IReflSaveTabView* m_view;
  std::unique_ptr<IReflAsciiSaver> m_saver;
  /// The main presenter
  IReflBatchPresenter *m_mainPresenter;
  bool m_shouldAutosave;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_CUSTOMINTERFACES_REFLSAVETABPRESENTER_H */
