// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "QSaveView.h"

#include <QFileDialog>
#include <QMessageBox>
#include <boost/algorithm/string.hpp>

namespace MantidQt {
namespace CustomInterfaces {

/** Constructor
 * @param parent :: The parent of this view
 */
QSaveView::QSaveView(QWidget *parent) : QWidget(parent), m_notifyee(nullptr) {
  initLayout();
}

void QSaveView::subscribe(SaveViewSubscriber *notifyee) {
  m_notifyee = notifyee;
  populateListOfWorkspaces();
  suggestSaveDir();
}

/** Destructor
 */
QSaveView::~QSaveView() {}

/**
Initialize the Interface
*/
void QSaveView::initLayout() {
  m_ui.setupUi(this);

  connect(m_ui.refreshButton, SIGNAL(clicked()), this,
          SLOT(populateListOfWorkspaces()));
  connect(m_ui.saveButton, SIGNAL(clicked()), this, SLOT(saveWorkspaces()));
  connect(m_ui.filterEdit, SIGNAL(textEdited(const QString &)), this,
          SLOT(filterWorkspaceList()));
  connect(m_ui.listOfWorkspaces, SIGNAL(itemDoubleClicked(QListWidgetItem *)),
          this, SLOT(requestWorkspaceParams()));
  connect(m_ui.saveReductionResultsCheckBox, SIGNAL(stateChanged(int)), this,
          SLOT(onAutosaveChanged(int)));
  connect(m_ui.savePathEdit, SIGNAL(editingFinished()), this,
          SLOT(onSavePathChanged()));
  connect(m_ui.savePathBrowseButton, SIGNAL(clicked()), this,
          SLOT(browseToSaveDirectory()));
}

void QSaveView::browseToSaveDirectory() {
  auto savePath = QFileDialog::getExistingDirectory(
      this, "Select the directory to save to.");
  if (!savePath.isEmpty()) {
    m_ui.savePathEdit->setText(savePath);
    onSavePathChanged();
  }
}

void QSaveView::onSavePathChanged() { m_notifyee->notifySavePathChanged(); }

void QSaveView::onAutosaveChanged(int state) {
  if (state == Qt::CheckState::Checked)
    m_notifyee->notifyAutosaveEnabled();
  else
    m_notifyee->notifyAutosaveDisabled();
}

void QSaveView::disableAutosaveControls() {
  m_ui.autosaveGroup->setEnabled(false);
}

void QSaveView::enableAutosaveControls() {
  m_ui.autosaveGroup->setEnabled(true);
}

void QSaveView::enableFileFormatAndLocationControls() {
  m_ui.fileFormatGroup->setEnabled(true);
  m_ui.fileLocationGroup->setEnabled(true);
}

void QSaveView::disableFileFormatAndLocationControls() {
  m_ui.fileFormatGroup->setEnabled(false);
  m_ui.fileLocationGroup->setEnabled(false);
}

/** Returns the save path
 * @return :: The save path
 */
std::string QSaveView::getSavePath() const {
  return m_ui.savePathEdit->text().toStdString();
}

/** Sets the save path
 */
void QSaveView::setSavePath(const std::string &path) const {
  m_ui.savePathEdit->setText(QString::fromStdString(path));
}

/** Returns the file name prefix
 * @return :: The prefix
 */
std::string QSaveView::getPrefix() const {
  return m_ui.prefixEdit->text().toStdString();
}

/** Returns the workspace list filter
 * @return :: The filter
 */
std::string QSaveView::getFilter() const {
  return m_ui.filterEdit->text().toStdString();
}

/** Returns the regular expression check value
 * @return :: The regex check
 */
bool QSaveView::getRegexCheck() const {
  return m_ui.regexCheckBox->isChecked();
}

/** Returns the name of the currently selected workspace from the 'List of
 * workspaces' widget
 * @return :: item name
 */
std::string QSaveView::getCurrentWorkspaceName() const {
  return m_ui.listOfWorkspaces->currentItem()->text().toStdString();
}

/** Returns a list of names of currently selected workspaces
 * @return :: workspace names
 */
std::vector<std::string> QSaveView::getSelectedWorkspaces() const {
  std::vector<std::string> itemNames;
  auto items = m_ui.listOfWorkspaces->selectedItems();
  for (auto it = items.begin(); it != items.end(); it++) {
    itemNames.push_back((*it)->text().toStdString());
  }
  return itemNames;
}

/** Returns a list of names of currently selected parameters
 * @return :: parameter names
 */
std::vector<std::string> QSaveView::getSelectedParameters() const {
  std::vector<std::string> paramNames;
  auto items = m_ui.listOfLoggedParameters->selectedItems();
  for (auto it = items.begin(); it != items.end(); it++) {
    paramNames.push_back((*it)->text().toStdString());
  }
  return paramNames;
}

/** Returns the index of the selected file format
 * @return :: File format index
 */
int QSaveView::getFileFormatIndex() const {
  return m_ui.fileFormatComboBox->currentIndex();
}

/** Returns the title check value
 * @return :: The title check
 */
bool QSaveView::getTitleCheck() const {
  return m_ui.titleCheckBox->isChecked();
}

/** Returns the Q resolution check value
 * @return :: The Q resolution check
 */
bool QSaveView::getQResolutionCheck() const {
  return m_ui.qResolutionCheckBox->isChecked();
}

void QSaveView::disallowAutosave() {
  m_ui.saveReductionResultsCheckBox->setCheckState(Qt::CheckState::Unchecked);
}

/** Returns the separator type
 * @return :: The separator
 */
std::string QSaveView::getSeparator() const {
  auto sep = m_ui.separatorButtonGroup->checkedButton()->text().toStdString();
  boost::to_lower(sep); // lowercase
  return sep;
}

/** Clear the 'List of workspaces' widget
 */
void QSaveView::clearWorkspaceList() const { m_ui.listOfWorkspaces->clear(); }

/** Clear the 'List of Logged Parameters' widget
 */
void QSaveView::clearParametersList() const {
  m_ui.listOfLoggedParameters->clear();
}

/** Set the 'List of workspaces' widget with workspace names
 * @param names :: The list of workspace names
 */
void QSaveView::setWorkspaceList(const std::vector<std::string> &names) const {
  for (auto it = names.begin(); it != names.end(); it++) {
    m_ui.listOfWorkspaces->addItem(QString::fromStdString(*it));
  }
}

/** Set the 'List of logged parameters' widget with workspace run logs
 * @param logs :: The list of workspace run logs
 */
void QSaveView::setParametersList(const std::vector<std::string> &logs) const {
  for (auto it = logs.begin(); it != logs.end(); it++) {
    m_ui.listOfLoggedParameters->addItem(QString::fromStdString(*it));
  }
}

/** Populate the 'List of workspaces' widget
 */
void QSaveView::populateListOfWorkspaces() const {
  m_notifyee->notifyPopulateWorkspaceList();
}

/** Filter the 'List of workspaces' widget
 */
void QSaveView::filterWorkspaceList() const {
  m_notifyee->notifyFilterWorkspaceList();
}

/** Request for the parameters of a workspace
 */
void QSaveView::requestWorkspaceParams() const {
  m_notifyee->notifyPopulateParametersList();
}

/** Save selected workspaces
 */
void QSaveView::saveWorkspaces() const {
  m_notifyee->notifySaveSelectedWorkspaces();
}

/** Suggest a save directory
 */
void QSaveView::suggestSaveDir() const { m_notifyee->notifySuggestSaveDir(); }

void QSaveView::error(const std::string &title, const std::string &prompt) {
  QMessageBox::critical(this, QString::fromStdString(title),
                        QString::fromStdString(prompt));
}

void QSaveView::warning(const std::string &title, const std::string &prompt) {
  QMessageBox::critical(this, QString::fromStdString(title),
                        QString::fromStdString(prompt));
}

void QSaveView::showFilterEditValid() {
  auto palette = m_ui.filterEdit->palette();
  palette.setColor(QPalette::Base, Qt::transparent);
  m_ui.filterEdit->setPalette(palette);
}

void QSaveView::showFilterEditInvalid() {
  auto palette = m_ui.filterEdit->palette();
  palette.setColor(QPalette::Base, QColor("#ffb8ad"));
  m_ui.filterEdit->setPalette(palette);
}

void QSaveView::errorInvalidSaveDirectory() {
  error("Invalid directory", "The save path specified doesn't exist or is "
                             "not writable.");
}

void QSaveView::warnInvalidSaveDirectory() {
  warning("Invalid directory",
          "You just changed the save path to a directory which "
          "doesn't exist or is not writable.");
}

void QSaveView::noWorkspacesSelected() {
  error("No workspaces selected.",
        "You must select the workspaces in order to save.");
}

void QSaveView::cannotSaveWorkspaces() {
  error("Error", "Unknown error while saving workspaces");
}

void QSaveView::cannotSaveWorkspaces(std::string const &fullError) {
  error("Error", fullError);
}
} // namespace CustomInterfaces
} // namespace MantidQt
