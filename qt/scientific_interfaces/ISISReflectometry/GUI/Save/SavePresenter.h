// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_SAVEPRESENTER_H
#define MANTID_CUSTOMINTERFACES_SAVEPRESENTER_H

#include "../../DllConfig.h"
#include "../../IReflAsciiSaver.h"
#include "ISavePresenter.h"
#include "ISaveView.h"
#include "MantidKernel/ConfigPropertyObserver.h"
#include <boost/optional.hpp>
#include <memory>
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

/** @class SavePresenter

SavePresenter is a presenter class for the tab 'Save ASCII' in the
ISIS Reflectometry Interface.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL SavePresenter : public ISavePresenter,
                                                     public SaveViewSubscriber {
public:
  SavePresenter(ISaveView *view, std::unique_ptr<IReflAsciiSaver> saver);

  void acceptMainPresenter(IReflBatchPresenter *mainPresenter) override;
  void notifyPopulateWorkspaceList() override;
  void notifyFilterWorkspaceList() override;
  void notifyPopulateParametersList() override;
  void notifySaveSelectedWorkspaces() override;
  void notifySuggestSaveDir() override;
  void notifyAutosaveDisabled() override;
  void notifyAutosaveEnabled() override;
  void notifySavePathChanged() override;

  void reductionCompletedForGroup(
      MantidWidgets::DataProcessor::GroupData const &group,
      std::string const &workspaceName) override;
  void
  reductionCompletedForRow(MantidWidgets::DataProcessor::GroupData const &group,
                           std::string const &workspaceName) override;
  void reductionPaused() override;
  void reductionResumed() override;
  void autoreductionPaused() override;
  void autoreductionResumed() override;

private:
  IReflBatchPresenter *m_mainPresenter;
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
  ISaveView *m_view;
  std::unique_ptr<IReflAsciiSaver> m_saver;
  bool m_shouldAutosave;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_CUSTOMINTERFACES_SAVEPRESENTER_H */
