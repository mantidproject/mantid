// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_REFLSAVETABPRESENTER_H
#define MANTID_CUSTOMINTERFACES_REFLSAVETABPRESENTER_H

#include "DllConfig.h"
#include "IReflAsciiSaver.h"
#include "IReflSaveTabPresenter.h"
#include "MantidKernel/ConfigPropertyObserver.h"
#include <boost/optional.hpp>
#include <memory>
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

// Forward decs
class IReflMainWindowPresenter;
class IReflSaveTabView;

/** @class ReflSaveTabPresenter

ReflSaveTabPresenter is a presenter class for the tab 'Save ASCII' in the
ISIS Reflectometry Interface.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL ReflSaveTabPresenter
    : public IReflSaveTabPresenter {
public:
  /// Constructor
  ReflSaveTabPresenter(std::unique_ptr<IReflAsciiSaver> saver,
                       std::unique_ptr<IReflSaveTabView> view);
  /// Destructor
  ~ReflSaveTabPresenter() override;
  /// Accept a main presenter
  void acceptMainPresenter(IReflMainWindowPresenter *mainPresenter) override;
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
  std::unique_ptr<IReflSaveTabView> m_view;
  std::unique_ptr<IReflAsciiSaver> m_saver;
  /// The main presenter
  IReflMainWindowPresenter *m_mainPresenter;
  bool m_shouldAutosave;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_CUSTOMINTERFACES_REFLSAVETABPRESENTER_H */
