// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "ui_DataSelector.h"

#include "MantidAPI/AlgorithmRuntimeProps.h"
#include "MantidQtWidgets/Common/MantidWidget.h"
#include "MantidQtWidgets/Common/QtAlgorithmRunner.h"

#include <QWidget>

namespace MantidQt {
namespace MantidWidgets {

using ButtonOpts = API::FileFinderWidget::ButtonOpts;
using LiveButtonOpts = API::FileFinderWidget::LiveButtonOpts;

/**
This class defines a widget for selecting a workspace of file path by using a
combination
of two child MantidWidgets: FileFinderWidget and WorkspaceSelector. This widget
combines the two to
produce a single composite widget that emits signals when the user has chosen
appropriate input.

@author Samuel Jackson
@date 07/08/2013
*/

class EXPORT_OPT_MANTIDQT_COMMON DataSelector : public API::MantidWidget {
  Q_OBJECT

  // These are properties of the file browser sub-widget
  Q_PROPERTY(QStringList fileBrowserSuffixes READ getFBSuffixes WRITE setFBSuffixes)
  Q_PROPERTY(bool showLoad READ willShowLoad WRITE setShowLoad)
  Q_PROPERTY(QString instrumentOverride READ getInstrumentOverride WRITE setInstrumentOverride)
  Q_PROPERTY(bool multipleFiles READ allowMultipleFiles WRITE allowMultipleFiles)
  Q_PROPERTY(bool findRunFiles READ isForRunFiles WRITE isForRunFiles)
  Q_PROPERTY(bool findDirectory READ isForDirectory WRITE isForDirectory)
  Q_PROPERTY(QString label READ getLabelText WRITE setLabelText)
  Q_PROPERTY(bool multiEntry READ doMultiEntry WRITE doMultiEntry)
  Q_PROPERTY(QString algorithmAndProperty READ getAlgorithmProperty WRITE setAlgorithmProperty)
  Q_PROPERTY(bool extsAsSingleOption READ extsAsSingleOption WRITE extsAsSingleOption)
  Q_PROPERTY(ButtonOpts buttonOpts READ doButtonOpt WRITE doButtonOpt)
  Q_PROPERTY(LiveButtonOpts liveButton READ liveButtonState WRITE liveButtonState)
  Q_ENUMS(ButtonOpts)
  Q_ENUMS(LiveButtonOpts)

  // These are properties of the workspace selector sub-widget
  Q_PROPERTY(QStringList workspaceSuffixes READ getWSSuffixes WRITE setWSSuffixes)
  Q_PROPERTY(QStringList WorkspaceTypes READ getWorkspaceTypes WRITE setWorkspaceTypes)
  Q_PROPERTY(bool ShowHidden READ showHiddenWorkspaces WRITE showHiddenWorkspaces)
  Q_PROPERTY(bool ShowGroups READ showWorkspaceGroups WRITE showWorkspaceGroups)
  Q_PROPERTY(QString Algorithm READ getValidatingAlgorithm WRITE setValidatingAlgorithm)

  // These are global properties of data selector
  Q_PROPERTY(bool optional READ isOptional WRITE isOptional)
  Q_PROPERTY(bool autoLoad READ willAutoLoad WRITE setAutoLoad)
  Q_PROPERTY(QString loadLabelText READ getLoadBtnText WRITE setLoadBtnText)

public:
  DataSelector(QWidget *parent = nullptr);
  ~DataSelector() override;

  /// Get the current file path in the FileFinderWidget widget
  QString getFullFilePath() const;
  /// Get the workspace name from the list of files
  QString getWsNameFromFiles() const;
  /// Get the currently available file or workspace name
  virtual QString getCurrentDataName() const;
  /// Sets which selector (file or workspace) is visible
  void setSelectorIndex(int index);
  /// Sets if the option to choose selector is visible
  void setTypeSelectorVisible(bool visible);
  /// Set the index of the combobox containing the loaded workspace
  void setWorkspaceSelectorIndex(QString const &workspaceName);
  /// Get whether the file selector is currently being shown
  bool isFileSelectorVisible() const;
  /// Get whether the workspace selector is currently being shown
  bool isWorkspaceSelectorVisible() const;
  /// Checks if widget is in a valid state
  virtual bool isValid();
  /// Get file problem, empty string means no error.
  QString getProblem() const;
  /// Read settings from the given group
  void readSettings(const QString & /*group*/);
  /// Save settings in the given group
  void saveSettings(const QString & /*group*/);
  /// Gets if optional
  bool isOptional() const;
  /// Sets if optional
  void isOptional(bool /*optional*/);
  /// Gets will auto load
  bool willAutoLoad() const;
  /// Sets will auto load
  void setAutoLoad(bool /*load*/);
  /// Check if the widget will show the load button
  bool willShowLoad();
  /// Set if the load button should be shown
  void setShowLoad(bool load);
  /// Gets the load button text
  QString getLoadBtnText() const;
  /// Sets the load button text
  void setLoadBtnText(const QString & /*text*/);
  /// Sets the DataSelector to always load data inside a WorkspaceGroup
  void setAlwaysLoadAsGroup(bool const loadAsGroup);
  /// Set an extra property on the load algorithm before execution
  void setLoadProperty(std::string const &propertyName, bool const value);

  // These are accessors/modifiers of the child FileFinderWidget
  /**
   * Return whether this widget allows multiple files to be specified within the
   * edit box
   * @returns True if multiple files can be specified, false otherwise
   */
  bool allowMultipleFiles() const { return m_uiForm.rfFileInput->allowMultipleFiles(); }

  /**
   * Set whether this widget allows multiple files to be specifed or not
   * @param allow :: If true then the widget will accept multiple files else
   * only
   * a
   * single file may be specified
   */
  void allowMultipleFiles(const bool allow) { m_uiForm.rfFileInput->allowMultipleFiles(allow); }

  /**
   * Returns if this widget is for run file searching or not
   * @returns True if this widget searches for run files, false otherwise
   */
  bool isForRunFiles() const { return m_uiForm.rfFileInput->isForRunFiles(); }

  /**
   * Sets whether this widget is for run file searching or not
   * @param mode :: True if this widget searches for run files, false otherwise
   */
  void isForRunFiles(const bool mode) { m_uiForm.rfFileInput->isForRunFiles(mode); }

  /**
   * Returns if this widget is for selecting a directory or not.
   * @return True if selecting a directory
   */
  bool isForDirectory() const { return m_uiForm.rfFileInput->isForDirectory(); }

  /**
   * Sets directory searching mode.
   * @param mode True to search for directories only
   */
  void isForDirectory(const bool mode) { m_uiForm.rfFileInput->isForDirectory(mode); }

  /**
   * Return the label text on the widget
   * @returns The current value of the text on the label
   */
  QString getLabelText() const { return m_uiForm.rfFileInput->getLabelText(); }

  /**
   * Set the text on the label
   * @param text :: A string giving the label to use for the text
   */
  void setLabelText(const QString &text) { m_uiForm.rfFileInput->setLabelText(text); }

  /**
   * Whether to find the number of entries in the file or assume (the
   * normal situation) of one entry
   * @return true if the widget is to look for multiple entries
   */
  bool doMultiEntry() const { return m_uiForm.rfFileInput->doMultiEntry(); }

  /**
   * Set to true to enable the period number box
   * @param multiEntry whether to show the multiperiod box
   */
  void doMultiEntry(const bool multiEntry) { m_uiForm.rfFileInput->doMultiEntry(multiEntry); }

  /**
   * Returns the algorithm name
   * @returns The algorithm name
   */
  QString getAlgorithmProperty() const { return m_uiForm.rfFileInput->getAlgorithmProperty(); }

  /**
   * Sets an algorithm name that can be tied to this widget
   * @param text :: The name of the algorithm and property in the form
   * [AlgorithmName|PropertyName]
   */
  void setAlgorithmProperty(const QString &text) { m_uiForm.rfFileInput->setAlgorithmProperty(text); }

  /**
   * Returns whether the file dialog should display the exts as a single list or
   * as multiple items
   * @return boolean
   */
  bool extsAsSingleOption() const { return m_uiForm.rfFileInput->extsAsSingleOption(); }

  /**
   * Sets whether the file dialog should display the exts as a single list or as
   * multiple items
   * @param value :: If true the file dialog wil contain a single entry will all
   * filters
   */
  void extsAsSingleOption(const bool value) { m_uiForm.rfFileInput->extsAsSingleOption(value); }

  /**
   * Gets the suffixes allowed by the file browser
   *
   * @return List of suffixes allowed by the file browser
   */
  QStringList getFBSuffixes() { return m_uiForm.rfFileInput->getFileExtensions(); }

  /**
   * Sets the suffixes allowed by the file browser
   *
   * @param suffixes :: List of suffixes allowed by the file browser
   */
  void setFBSuffixes(const QStringList &suffixes) { m_uiForm.rfFileInput->setFileExtensions(suffixes); }

  /**
   * Gets the instrument override
   *
   * @return List of instrument override
   */
  QString getInstrumentOverride() { return m_uiForm.rfFileInput->getInstrumentOverride(); }

  /**
   * Sets the instrument override
   *
   * @param instName :: name of instrument override
   */
  void setInstrumentOverride(const QString &instName) { m_uiForm.rfFileInput->setInstrumentOverride(instName); }

  /**
   * Returns the preference for how the dialog control should be
   * @return the setting
   */
  ButtonOpts doButtonOpt() const { return m_uiForm.rfFileInput->doButtonOpt(); }

  /**
   * Set how the browse should appear
   * @param buttonOpt :: the preference for the control, if there will be one,
   * to activate the dialog box
   */

  void doButtonOpt(const ButtonOpts buttonOpt) { m_uiForm.rfFileInput->doButtonOpt(buttonOpt); }

  /**
   * Gets the live button state
   *
   * @return live button option
   */
  LiveButtonOpts liveButtonState() const { return m_uiForm.rfFileInput->liveButtonState(); }

  /**
   * Sets the live button state
   *
   * @param option :: livebutton option
   */
  void liveButtonState(const LiveButtonOpts option) { m_uiForm.rfFileInput->liveButtonState(option); }

  // These are accessors/modifiers of the child WorkspaceSelector
  /**
   * Gets the suffixes allowed by the workspace selector
   *
   * @return List of suffixes allowed by the workspace selector
   */
  QStringList getWSSuffixes() { return m_uiForm.wsWorkspaceInput->getSuffixes(); }

  /**
   * Sets the suffixes allowed by the workspace selector
   *
   * @param suffixes :: List of suffixes allowed by the workspace selector
   */
  void setWSSuffixes(const QStringList &suffixes) { m_uiForm.wsWorkspaceInput->setSuffixes(suffixes); }

  /**
   * Gets the workspace types allowed by the workspace selector
   *
   * @return List of workspace types allowed by the workspace selector
   */
  QStringList getWorkspaceTypes() const { return m_uiForm.wsWorkspaceInput->getWorkspaceTypes(); }

  /**
   * Sets the workspace types allowed by the workspace selector
   *
   * @param types :: List of workspace types allowed by the workspace selector
   */
  void setWorkspaceTypes(const QStringList &types) { m_uiForm.wsWorkspaceInput->setWorkspaceTypes(types); }

  /**
   * Gets if the workspace selector shows hidden workspaces
   *
   * @return Boolean flag if hidden workspaces are shown
   */
  bool showHiddenWorkspaces() const { return m_uiForm.wsWorkspaceInput->showHiddenWorkspaces(); }

  /**
   * Sets if the workspace selector shows hidden workspaces
   *
   * @param show :: Boolean flag if hidden workspaces are shown
   */
  void showHiddenWorkspaces(bool show) { m_uiForm.wsWorkspaceInput->showHiddenWorkspaces(show); }

  /**
   * Gets if the workspace selector shows group workspaces
   *
   * @return Boolean flag if group workspaces are shown
   */
  bool showWorkspaceGroups() const { return m_uiForm.wsWorkspaceInput->showWorkspaceGroups(); }

  /**
   * Sets if the workspace selector shows workspace groups
   *
   * @param show :: Boolean flag if group workspaces are shown
   */
  void showWorkspaceGroups(bool show) { m_uiForm.wsWorkspaceInput->showWorkspaceGroups(show); }

  /**
   * Gets if the validating algorithm of workspace selector
   *
   * @return validating algorithm
   */
  QString getValidatingAlgorithm() const { return m_uiForm.wsWorkspaceInput->getValidatingAlgorithm(); }

  /**
   * Sets the validating algorithm of workspace selector
   *
   * @param algName :: validating algorithm
   */
  void setValidatingAlgorithm(const QString &algName) { m_uiForm.wsWorkspaceInput->setValidatingAlgorithm(algName); }

signals:
  /// Signal emitted when files were found but widget isn't autoloading
  void filesFound();
  /// Signal emitted when file input is visible
  void fileViewVisible();
  /// Signal emitted when workspace selector is visible
  void workspaceViewVisible();
  /// Signal emitted when data is ready from a workspace selector or file
  /// browser
  void dataReady(const QString &wsname);
  /// Signal emitted when the load button is clicked
  void loadClicked();
  /// Signal emitted when files are found and autoloaded
  void filesAutoLoaded();

protected:
  // Method for handling drop events
  void dropEvent(QDropEvent * /*unused*/) override;
  // called when a drag event enters the class
  void dragEnterEvent(QDragEnterEvent * /*unused*/) override;

private slots:
  /// Slot called when the current view is changed
  void handleViewChanged(int index);
  /// Slot called when file input is available
  void handleFileInput();
  /// Slot called when workspace input is available
  void handleWorkspaceInput();
  /// Slot called if the widget fails to auto load the file.
  void handleAutoLoadComplete(bool error);

private:
  /// Attempt to automatically load a file
  void autoLoadFile(const QString &filenames);
  /// Execute load algorithm
  void executeLoadAlgorithm(std::string const &filename, std::string const &outputWorkspace);
  /// Member containing the widgets child widgets.
  Ui::DataSelector m_uiForm;
  /// Extra load properties to set on the load algorithm before execution
  Mantid::API::AlgorithmRuntimeProps m_loadProperties;
  /// Algorithm Runner used to run the load algorithm
  MantidQt::API::QtAlgorithmRunner m_algRunner;
  /// Flag to enable auto loading. By default this is set to true.
  bool m_autoLoad;
  /// Flag to show or hide the load button. By default this is set to true.
  bool m_showLoad;
  /// Flag if optional
  bool m_isOptional;
  /// Flag to always load data, placing it inside a WorkspaceGroup, even if there is 1 entry
  bool m_alwaysLoadAsGroup;
};

} /* namespace MantidWidgets */
} /* namespace MantidQt */
