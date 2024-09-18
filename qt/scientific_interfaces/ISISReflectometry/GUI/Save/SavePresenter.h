// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "IFileSaver.h"
#include "ISavePresenter.h"
#include "ISaveView.h"
#include "MantidKernel/ConfigPropertyObserver.h"
#include <boost/optional.hpp>
#include <memory>
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/** @class SavePresenter

    SavePresenter is a presenter class for the tab 'Save ASCII' in the
    ISIS Reflectometry Interface.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL SavePresenter : public ISavePresenter, public SaveViewSubscriber {
public:
  SavePresenter(ISaveView *view, std::unique_ptr<IFileSaver> saver);

  // ISavePresenter overrides
  void acceptMainPresenter(IBatchPresenter *mainPresenter) override;
  void saveWorkspaces(std::vector<std::string> const &workspaceNames, bool const isAutoSave = false) override;
  bool shouldAutosave() const override;
  bool shouldAutosaveGroupRows() const override;
  void notifyReductionPaused() override;
  void notifyReductionResumed() override;
  void notifyAutoreductionPaused() override;
  void notifyAutoreductionResumed() override;

  // SaveViewSubscriber overrides
  void notifySettingsChanged() override;
  void notifyPopulateWorkspaceList() override;
  void notifyFilterWorkspaceList() override;
  void notifyPopulateParametersList() override;
  void notifySaveSelectedWorkspaces() override;
  void notifyAutosaveDisabled() override;
  void notifyAutosaveEnabled() override;
  void notifySaveIndividualRowsEnabled() override;
  void notifySaveIndividualRowsDisabled() override;
  void notifySavePathChanged() override;

private:
  IBatchPresenter *m_mainPresenter;
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
  void saveWorkspaces(std::vector<std::string> const &workspaceNames, std::vector<std::string> const &logParameters,
                      bool const isAutoSave);
  /// Obtains all available workspace names
  std::vector<std::string> getAvailableWorkspaceNames();
  NamedFormat formatFromIndex(int formatIndex) const;
  FileFormatOptions getSaveParametersFromView(bool const isAutoSave) const;
  void enableAutosave();
  void disableAutosave();
  void updateWidgetEnabledState() const;
  void updateWidgetStateBasedOnFileFormat() const;
  bool isProcessing() const;
  bool isAutoreducing() const;
  bool hasSelectedORSOFormat() const;

  /// The view
  ISaveView *m_view;
  std::unique_ptr<IFileSaver> m_saver;
  bool m_shouldAutosave;
  bool m_shouldSaveIndividualRows;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
