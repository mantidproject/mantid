// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_SAVEVIEW_H_
#define MANTID_CUSTOMINTERFACES_SAVEVIEW_H_

#include "ISaveView.h"
#include "ui_SaveWidget.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

/** SaveView : Provides an interface for the "Save ASCII" tab in the
ISIS Reflectometry interface.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL SaveView : public QWidget,
                                                public ISaveView {
  Q_OBJECT
public:
  /// Constructor
  explicit SaveView(QWidget *parent = nullptr);
  /// Destructor
  ~SaveView() override;

  void subscribe(SaveViewSubscriber *notifyee) override;

  /// Returns the save path
  std::string getSavePath() const override;
  /// Sets the save path
  void setSavePath(const std::string &path) const override;
  /// Returns the prefix
  std::string getPrefix() const override;
  /// Returns the filter
  std::string getFilter() const override;
  /// Returns the regex check
  bool getRegexCheck() const override;
  /// Get name of the currently selected workspace name
  std::string getCurrentWorkspaceName() const override;
  /// Returns a list of names of selected workspaces
  std::vector<std::string> getSelectedWorkspaces() const override;
  /// Returns a list of names of selected parameters
  std::vector<std::string> getSelectedParameters() const override;
  /// Returns the index of selected file format
  int getFileFormatIndex() const override;
  /// Returns the title check
  bool getTitleCheck() const override;
  /// Returns the Q resolution check
  bool getQResolutionCheck() const override;
  /// Returns the separator type
  std::string getSeparator() const override;

  /// Clears the 'List of Workspaces' widget
  void clearWorkspaceList() const override;
  /// Clears the 'List of Logged Parameters' widget
  void clearParametersList() const override;
  /// Sets the 'List of workspaces' widget
  void
  setWorkspaceList(const std::vector<std::string> & /*unused*/) const override;
  /// Sets the 'List of logged parameters' widget
  void
  setParametersList(const std::vector<std::string> & /*unused*/) const override;

  void disallowAutosave() override;

  void disableAutosaveControls() override;
  void enableAutosaveControls() override;

  void enableFileFormatAndLocationControls() override;
  void disableFileFormatAndLocationControls() override;

  void error(const std::string &title, const std::string &prompt);
  void warning(const std::string &title, const std::string &prompt);

  void showFilterEditValid() override;
  void showFilterEditInvalid() override;
  void errorInvalidSaveDirectory() override;
  void warnInvalidSaveDirectory() override;
  void noWorkspacesSelected() override;
  void cannotSaveWorkspaces() override;
  void cannotSaveWorkspaces(std::string const &fullError) override;

public slots:
  /// Populate the 'List of workspaces' widget
  void populateListOfWorkspaces() const;
  /// Filters the 'List of workspaces' widget
  void filterWorkspaceList() const;
  /// Request parameters for a workspace of a name
  void requestWorkspaceParams() const;
  /// Save selected workspaces
  void saveWorkspaces() const;
  /// Suggest a save directory
  void suggestSaveDir() const;
  void browseToSaveDirectory();

  void onSavePathChanged();
  void onAutosaveChanged(int state);

private:
  /// Initialize the interface
  void initLayout();

  /// The widget
  Ui::SaveWidget m_ui;
  SaveViewSubscriber *m_notifyee;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_CUSTOMINTERFACES_SAVEVIEW_H_ */
