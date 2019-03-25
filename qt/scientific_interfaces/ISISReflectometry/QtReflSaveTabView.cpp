// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "QtReflSaveTabView.h"
#include "ReflSaveTabPresenter.h"

#include <QFileDialog>
#include <QMessageBox>
#include <boost/algorithm/string.hpp>

namespace MantidQt {
namespace CustomInterfaces {

/** Constructor
 * @param parent :: The parent of this view
 */
QtReflSaveTabView::QtReflSaveTabView(QWidget *parent) : m_presenter(nullptr) {
  UNUSED_ARG(parent);
  initLayout();
}

void QtReflSaveTabView::subscribe(IReflSaveTabPresenter *presenter) {
  m_presenter = presenter;
  populateListOfWorkspaces();
  suggestSaveDir();
}

/** Destructor
 */
QtReflSaveTabView::~QtReflSaveTabView() {}

/**
Initialize the Interface
*/
void QtReflSaveTabView::initLayout() {
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

void QtReflSaveTabView::browseToSaveDirectory() {
  auto savePath = QFileDialog::getExistingDirectory(
      this, "Select the directory to save to.");
  if (!savePath.isEmpty()) {
    m_ui.savePathEdit->setText(savePath);
    onSavePathChanged();
  }
}

void QtReflSaveTabView::onSavePathChanged() {
  m_presenter->notify(IReflSaveTabPresenter::Flag::savePathChanged);
}

void QtReflSaveTabView::onAutosaveChanged(int state) {
  if (state == Qt::CheckState::Checked)
    m_presenter->notify(IReflSaveTabPresenter::Flag::autosaveEnabled);
  else
    m_presenter->notify(IReflSaveTabPresenter::Flag::autosaveDisabled);
}

void QtReflSaveTabView::disableAutosaveControls() {
  m_ui.autosaveGroup->setEnabled(false);
}

void QtReflSaveTabView::enableAutosaveControls() {
  m_ui.autosaveGroup->setEnabled(true);
}

void QtReflSaveTabView::enableFileFormatAndLocationControls() {
  m_ui.fileFormatGroup->setEnabled(true);
  m_ui.fileLocationGroup->setEnabled(true);
}

void QtReflSaveTabView::disableFileFormatAndLocationControls() {
  m_ui.fileFormatGroup->setEnabled(false);
  m_ui.fileLocationGroup->setEnabled(false);
}

/** Returns the save path
 * @return :: The save path
 */
std::string QtReflSaveTabView::getSavePath() const {
  return m_ui.savePathEdit->text().toStdString();
}

/** Sets the save path
 */
void QtReflSaveTabView::setSavePath(const std::string &path) const {
  m_ui.savePathEdit->setText(QString::fromStdString(path));
}

/** Returns the file name prefix
 * @return :: The prefix
 */
std::string QtReflSaveTabView::getPrefix() const {
  return m_ui.prefixEdit->text().toStdString();
}

/** Returns the workspace list filter
 * @return :: The filter
 */
std::string QtReflSaveTabView::getFilter() const {
  return m_ui.filterEdit->text().toStdString();
}

/** Returns the regular expression check value
 * @return :: The regex check
 */
bool QtReflSaveTabView::getRegexCheck() const {
  return m_ui.regexCheckBox->isChecked();
}

/** Returns the name of the currently selected workspace from the 'List of
 * workspaces' widget
 * @return :: item name
 */
std::string QtReflSaveTabView::getCurrentWorkspaceName() const {
  return m_ui.listOfWorkspaces->currentItem()->text().toStdString();
}

/** Returns a list of names of currently selected workspaces
 * @return :: workspace names
 */
std::vector<std::string> QtReflSaveTabView::getSelectedWorkspaces() const {
  std::vector<std::string> itemNames;
  auto items = m_ui.listOfWorkspaces->selectedItems();
  for (auto &item : items) {
    itemNames.push_back(item->text().toStdString());
  }
  return itemNames;
}

/** Returns a list of names of currently selected parameters
 * @return :: parameter names
 */
std::vector<std::string> QtReflSaveTabView::getSelectedParameters() const {
  std::vector<std::string> paramNames;
  auto items = m_ui.listOfLoggedParameters->selectedItems();
  for (auto &item : items) {
    paramNames.push_back(item->text().toStdString());
  }
  return paramNames;
}

/** Returns the index of the selected file format
 * @return :: File format index
 */
int QtReflSaveTabView::getFileFormatIndex() const {
  return m_ui.fileFormatComboBox->currentIndex();
}

/** Returns the title check value
 * @return :: The title check
 */
bool QtReflSaveTabView::getTitleCheck() const {
  return m_ui.titleCheckBox->isChecked();
}

/** Returns the Q resolution check value
 * @return :: The Q resolution check
 */
bool QtReflSaveTabView::getQResolutionCheck() const {
  return m_ui.qResolutionCheckBox->isChecked();
}

void QtReflSaveTabView::disallowAutosave() {
  m_ui.saveReductionResultsCheckBox->setCheckState(Qt::CheckState::Unchecked);
}

/** Returns the separator type
 * @return :: The separator
 */
std::string QtReflSaveTabView::getSeparator() const {
  auto sep = m_ui.separatorButtonGroup->checkedButton()->text().toStdString();
  boost::to_lower(sep); // lowercase
  return sep;
}

/** Clear the 'List of workspaces' widget
 */
void QtReflSaveTabView::clearWorkspaceList() const {
  m_ui.listOfWorkspaces->clear();
}

/** Clear the 'List of Logged Parameters' widget
 */
void QtReflSaveTabView::clearParametersList() const {
  m_ui.listOfLoggedParameters->clear();
}

/** Set the 'List of workspaces' widget with workspace names
 * @param names :: The list of workspace names
 */
void QtReflSaveTabView::setWorkspaceList(
    const std::vector<std::string> &names) const {
  for (const auto &name : names) {
    m_ui.listOfWorkspaces->addItem(QString::fromStdString(name));
  }
}

/** Set the 'List of logged parameters' widget with workspace run logs
 * @param logs :: The list of workspace run logs
 */
void QtReflSaveTabView::setParametersList(
    const std::vector<std::string> &logs) const {
  for (const auto &log : logs) {
    m_ui.listOfLoggedParameters->addItem(QString::fromStdString(log));
  }
}

/** Populate the 'List of workspaces' widget
 */
void QtReflSaveTabView::populateListOfWorkspaces() const {
  m_presenter->notify(IReflSaveTabPresenter::populateWorkspaceListFlag);
}

/** Filter the 'List of workspaces' widget
 */
void QtReflSaveTabView::filterWorkspaceList() const {
  m_presenter->notify(IReflSaveTabPresenter::filterWorkspaceListFlag);
}

/** Request for the parameters of a workspace
 */
void QtReflSaveTabView::requestWorkspaceParams() const {
  m_presenter->notify(IReflSaveTabPresenter::workspaceParamsFlag);
}

/** Save selected workspaces
 */
void QtReflSaveTabView::saveWorkspaces() const {
  m_presenter->notify(IReflSaveTabPresenter::saveWorkspacesFlag);
}

/** Suggest a save directory
 */
void QtReflSaveTabView::suggestSaveDir() const {
  m_presenter->notify(IReflSaveTabPresenter::suggestSaveDirFlag);
}

/**
Show an critical error dialog
@param prompt : The prompt to appear on the dialog
@param title : The text for the title bar of the dialog
*/
void QtReflSaveTabView::giveUserCritical(const std::string &prompt,
                                         const std::string &title) {
  QMessageBox::critical(this, QString::fromStdString(title),
                        QString::fromStdString(prompt), QMessageBox::Ok,
                        QMessageBox::Ok);
}

/**
Show an information dialog
@param prompt : The prompt to appear on the dialog
@param title : The text for the title bar of the dialog
*/
void QtReflSaveTabView::giveUserInfo(const std::string &prompt,
                                     const std::string &title) {
  QMessageBox::information(this, QString::fromStdString(title),
                           QString::fromStdString(prompt), QMessageBox::Ok,
                           QMessageBox::Ok);
}
} // namespace CustomInterfaces
} // namespace MantidQt
