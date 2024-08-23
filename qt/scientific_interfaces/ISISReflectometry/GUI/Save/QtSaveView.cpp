// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "QtSaveView.h"
#include "MantidKernel/UsageService.h"

#include <QFileDialog>
#include <QMessageBox>
#include <boost/algorithm/string.hpp>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

/** Constructor
 * @param parent :: The parent of this view
 */
QtSaveView::QtSaveView(QWidget *parent) : QWidget(parent), m_notifyee(nullptr) { initLayout(); }

void QtSaveView::subscribe(SaveViewSubscriber *notifyee) { m_notifyee = notifyee; }

/** Destructor
 */
QtSaveView::~QtSaveView() = default;

/**
Initialize the Interface
*/
void QtSaveView::initLayout() {
  m_ui.setupUi(this);
  connect(m_ui.refreshButton, SIGNAL(clicked()), this, SLOT(populateListOfWorkspaces()));
  connect(m_ui.saveButton, SIGNAL(clicked()), this, SLOT(saveWorkspaces()));
  connect(m_ui.filterEdit, SIGNAL(textChanged(const QString &)), this, SLOT(filterWorkspaceList()));
  connect(m_ui.listOfWorkspaces, SIGNAL(itemDoubleClicked(QListWidgetItem *)), this, SLOT(requestWorkspaceParams()));
  connect(m_ui.saveReductionResultsCheckBox, SIGNAL(stateChanged(int)), this, SLOT(onAutosaveChanged(int)));
  connect(m_ui.saveIndividualRowsCheckBox, SIGNAL(stateChanged(int)), this, SLOT(onSaveIndividualRowsChanged(int)));
  connect(m_ui.savePathEdit, SIGNAL(editingFinished()), this, SLOT(onSavePathChanged()));
  connect(m_ui.savePathBrowseButton, SIGNAL(clicked()), this, SLOT(browseToSaveDirectory()));
}

void QtSaveView::connectSettingsChange(QLineEdit &edit) {
  connect(&edit, SIGNAL(textChanged(QString const &)), this, SLOT(onSettingsChanged()));
}

void QtSaveView::connectSettingsChange(QComboBox &edit) {
  connect(&edit, SIGNAL(currentIndexChanged(int)), this, SLOT(onSettingsChanged()));
}

void QtSaveView::connectSettingsChange(QCheckBox &edit) {
  connect(&edit, SIGNAL(stateChanged(int)), this, SLOT(onSettingsChanged()));
}

void QtSaveView::connectSettingsChange(QRadioButton &edit) {
  connect(&edit, SIGNAL(clicked()), this, SLOT(onSettingsChanged()));
}

void QtSaveView::onSettingsChanged() { m_notifyee->notifySettingsChanged(); }

void QtSaveView::connectSaveSettingsWidgets() {
  connectSettingsChange(*m_ui.savePathEdit);
  connectSettingsChange(*m_ui.prefixEdit);
  connectSettingsChange(*m_ui.filterEdit);
  connectSettingsChange(*m_ui.regexCheckBox);
  connectSettingsChange(*m_ui.saveReductionResultsCheckBox);
  connectSettingsChange(*m_ui.saveIndividualRowsCheckBox);
  connectSettingsChange(*m_ui.headerCheckBox);
  connectSettingsChange(*m_ui.qResolutionCheckBox);
  connectSettingsChange(*m_ui.extraColumnsCheckBox);
  connectSettingsChange(*m_ui.multipleDatasetsCheckBox);
  connectSettingsChange(*m_ui.commaRadioButton);
  connectSettingsChange(*m_ui.spaceRadioButton);
  connectSettingsChange(*m_ui.tabRadioButton);
  connectSettingsChange(*m_ui.fileFormatComboBox);
}

void QtSaveView::browseToSaveDirectory() {
  auto savePath = QFileDialog::getExistingDirectory(this, "Select the directory to save to.");
  if (!savePath.isEmpty()) {
    m_ui.savePathEdit->setText(savePath);
    onSavePathChanged();
  }
}

void QtSaveView::onSavePathChanged() {
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      Mantid::Kernel::FeatureType::Feature, {"ISIS Reflectometry", "SaveTab", "SavePathChanged"}, false);
  m_notifyee->notifySavePathChanged();
}

void QtSaveView::onAutosaveChanged(int state) {
  if (state == Qt::CheckState::Checked) {
    Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
        Mantid::Kernel::FeatureType::Feature, {"ISIS Reflectometry", "SaveTab", "EnableAutosave"}, false);
    m_notifyee->notifyAutosaveEnabled();
  } else {
    Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
        Mantid::Kernel::FeatureType::Feature, {"ISIS Reflectometry", "SaveTab", "DisableAutosave"}, false);
    m_notifyee->notifyAutosaveDisabled();
  }
}

void QtSaveView::onSaveIndividualRowsChanged(int state) {
  if (state == Qt::CheckState::Checked) {
    m_notifyee->notifySaveIndividualRowsEnabled();
  } else {
    m_notifyee->notifySaveIndividualRowsDisabled();
  }
}

void QtSaveView::disableAutosaveControls() { m_ui.autosaveGroup->setEnabled(false); }

void QtSaveView::enableAutosaveControls() { m_ui.autosaveGroup->setEnabled(true); }

void QtSaveView::enableFileFormatControls() { m_ui.fileFormatGroup->setEnabled(true); }

void QtSaveView::disableFileFormatControls() { m_ui.fileFormatGroup->setEnabled(false); }

void QtSaveView::enableLocationControls() { m_ui.fileLocationGroup->setEnabled(true); }

void QtSaveView::disableLocationControls() { m_ui.fileLocationGroup->setEnabled(false); }

void QtSaveView::enableLogList() { m_ui.listOfLoggedParameters->setEnabled(true); }

void QtSaveView::disableLogList() { m_ui.listOfLoggedParameters->setEnabled(false); }

void QtSaveView::enableHeaderCheckBox() { m_ui.headerCheckBox->setEnabled(true); }

void QtSaveView::disableHeaderCheckBox() { m_ui.headerCheckBox->setEnabled(false); }

void QtSaveView::enableQResolutionCheckBox() { m_ui.qResolutionCheckBox->setEnabled(true); }

void QtSaveView::disableQResolutionCheckBox() { m_ui.qResolutionCheckBox->setEnabled(false); }

void QtSaveView::enableAdditionalColumnsCheckBox() { m_ui.extraColumnsCheckBox->setEnabled(true); }

void QtSaveView::disableAdditionalColumnsCheckBox() { m_ui.extraColumnsCheckBox->setEnabled(false); }

void QtSaveView::enableSeparatorButtonGroup() {
  m_ui.commaRadioButton->setEnabled(true);
  m_ui.spaceRadioButton->setEnabled(true);
  m_ui.tabRadioButton->setEnabled(true);
}

void QtSaveView::disableSeparatorButtonGroup() {
  m_ui.commaRadioButton->setEnabled(false);
  m_ui.spaceRadioButton->setEnabled(false);
  m_ui.tabRadioButton->setEnabled(false);
}
void QtSaveView::enableSaveToSingleFileCheckBox() { m_ui.multipleDatasetsCheckBox->setEnabled(true); }

void QtSaveView::disableSaveToSingleFileCheckBox() { m_ui.multipleDatasetsCheckBox->setEnabled(false); }

void QtSaveView::enableSaveIndividualRowsCheckbox() { m_ui.saveIndividualRowsCheckBox->setEnabled(true); }

void QtSaveView::disableSaveIndividualRowsCheckbox() { m_ui.saveIndividualRowsCheckBox->setEnabled(false); }

/** Returns the save path
 * @return :: The save path
 */
std::string QtSaveView::getSavePath() const { return m_ui.savePathEdit->text().toStdString(); }

/** Sets the save path
 */
void QtSaveView::setSavePath(const std::string &path) const {
  m_ui.savePathEdit->setText(QString::fromStdString(path));
}

/** Returns the file name prefix
 * @return :: The prefix
 */
std::string QtSaveView::getPrefix() const { return m_ui.prefixEdit->text().toStdString(); }

/** Returns the workspace list filter
 * @return :: The filter
 */
std::string QtSaveView::getFilter() const { return m_ui.filterEdit->text().toStdString(); }

/** Returns the regular expression check value
 * @return :: The regex check
 */
bool QtSaveView::getRegexCheck() const { return m_ui.regexCheckBox->isChecked(); }

/** Returns the name of the currently selected workspace from the 'List of
 * workspaces' widget
 * @return :: item name
 */
std::string QtSaveView::getCurrentWorkspaceName() const {
  return m_ui.listOfWorkspaces->currentItem()->text().toStdString();
}

/** Returns a list of names of currently selected workspaces
 * @return :: workspace names
 */
std::vector<std::string> QtSaveView::getSelectedWorkspaces() const {
  std::vector<std::string> itemNames;
  auto items = m_ui.listOfWorkspaces->selectedItems();
  for (auto it = items.begin(); it != items.end(); it++) {
    itemNames.emplace_back((*it)->text().toStdString());
  }
  return itemNames;
}

/** Returns a list of names of currently selected parameters
 * @return :: parameter names
 */
std::vector<std::string> QtSaveView::getSelectedParameters() const {
  std::vector<std::string> paramNames;
  auto items = m_ui.listOfLoggedParameters->selectedItems();
  for (auto it = items.begin(); it != items.end(); it++) {
    paramNames.emplace_back((*it)->text().toStdString());
  }
  return paramNames;
}

/** Returns the index of the selected file format
 * @return :: File format index
 */
int QtSaveView::getFileFormatIndex() const { return m_ui.fileFormatComboBox->currentIndex(); }

/** Returns the header check value
 * @return :: The header check
 */
bool QtSaveView::getHeaderCheck() const { return m_ui.headerCheckBox->isChecked(); }

/** Returns the Q resolution check value
 * @return :: The Q resolution check
 */
bool QtSaveView::getQResolutionCheck() const { return m_ui.qResolutionCheckBox->isChecked(); }

/** Returns the include additional columns check value
 * @return :: The include additional columns check
 */
bool QtSaveView::getAdditionalColumnsCheck() const { return m_ui.extraColumnsCheckBox->isChecked(); }

void QtSaveView::disallowAutosave() { m_ui.saveReductionResultsCheckBox->setCheckState(Qt::CheckState::Unchecked); }

/** Returns the separator type
 * @return :: The separator
 */
std::string QtSaveView::getSeparator() const {
  auto sep = m_ui.separatorButtonGroup->checkedButton()->text().toStdString();
  boost::to_lower(sep); // lowercase
  return sep;
}

/** Returns the save multiple datasets to single file check value
 * @return :: The save multiple datasets to single file check
 */
bool QtSaveView::getSaveToSingleFileCheck() const { return m_ui.multipleDatasetsCheckBox->isChecked(); }

/** Clear the 'List of workspaces' widget
 */
void QtSaveView::clearWorkspaceList() const { m_ui.listOfWorkspaces->clear(); }

/** Clear the 'List of Logged Parameters' widget
 */
void QtSaveView::clearParametersList() const { m_ui.listOfLoggedParameters->clear(); }

/** Set the 'List of workspaces' widget with workspace names
 * @param names :: The list of workspace names
 */
void QtSaveView::setWorkspaceList(const std::vector<std::string> &names) const {
  for (auto it = names.begin(); it != names.end(); it++) {
    m_ui.listOfWorkspaces->addItem(QString::fromStdString(*it));
  }
}

/** Set the 'List of logged parameters' widget with workspace run logs
 * @param logs :: The list of workspace run logs
 */
void QtSaveView::setParametersList(const std::vector<std::string> &logs) const {
  for (auto it = logs.begin(); it != logs.end(); it++) {
    m_ui.listOfLoggedParameters->addItem(QString::fromStdString(*it));
  }
}

/** Populate the 'List of workspaces' widget
 */
void QtSaveView::populateListOfWorkspaces() const {
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      Mantid::Kernel::FeatureType::Feature, {"ISIS Reflectometry", "SaveTab", "PopulateWorkspaces"}, false);
  m_notifyee->notifyPopulateWorkspaceList();
}

/** Filter the 'List of workspaces' widget
 */
void QtSaveView::filterWorkspaceList() const { m_notifyee->notifyFilterWorkspaceList(); }

/** Request for the parameters of a workspace
 */
void QtSaveView::requestWorkspaceParams() const {
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      Mantid::Kernel::FeatureType::Feature, {"ISIS Reflectometry", "SaveTab", "PopulateParameters"}, false);
  m_notifyee->notifyPopulateParametersList();
}

/** Save selected workspaces
 */
void QtSaveView::saveWorkspaces() const {
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      Mantid::Kernel::FeatureType::Feature, {"ISIS Reflectometry", "SaveTab", "SaveWorkspaces"}, false);
  m_notifyee->notifySaveSelectedWorkspaces();
}

void QtSaveView::error(const std::string &title, const std::string &prompt) {
  QMessageBox::critical(this, QString::fromStdString(title), QString::fromStdString(prompt));
}

void QtSaveView::warning(const std::string &title, const std::string &prompt) {
  QMessageBox::critical(this, QString::fromStdString(title), QString::fromStdString(prompt));
}

void QtSaveView::showFilterEditValid() {
  auto palette = m_ui.filterEdit->palette();
  palette.setColor(QPalette::Base, Qt::transparent);
  m_ui.filterEdit->setPalette(palette);
}

void QtSaveView::showFilterEditInvalid() {
  auto palette = m_ui.filterEdit->palette();
  palette.setColor(QPalette::Base, QColor("#ffb8ad"));
  m_ui.filterEdit->setPalette(palette);
}

void QtSaveView::errorInvalidSaveDirectory() {
  error("Invalid directory", "The save path specified doesn't exist or is "
                             "not writable.");
}

void QtSaveView::warnInvalidSaveDirectory() {
  warning("Invalid directory", "You just changed the save path to a directory which "
                               "doesn't exist or is not writable.");
}

void QtSaveView::noWorkspacesSelected() {
  error("No workspaces selected.", "You must select the workspaces in order to save.");
}

void QtSaveView::cannotSaveWorkspaces() { error("Error", "Unknown error while saving workspaces"); }

void QtSaveView::cannotSaveWorkspaces(std::string const &fullError) { error("Error", fullError); }
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
