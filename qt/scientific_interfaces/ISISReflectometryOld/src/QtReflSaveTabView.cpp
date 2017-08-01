#include "MantidQtCustomInterfaces/Reflectometry/QtReflSaveTabView.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflSaveTabPresenter.h"

#include <boost/algorithm/string.hpp>

namespace MantidQt {
namespace CustomInterfaces {

/** Constructor
* @param parent :: The parent of this view
*/
QtReflSaveTabView::QtReflSaveTabView(QWidget *parent) : m_presenter() {

  UNUSED_ARG(parent);
  initLayout();
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

  m_presenter.reset(new ReflSaveTabPresenter(this));
  populateListOfWorkspaces();
  suggestSaveDir();
}

/** Returns the presenter managing this view
* @return :: A pointer to the presenter
*/
IReflSaveTabPresenter *QtReflSaveTabView::getPresenter() const {

  return m_presenter.get();
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
  for (auto it = items.begin(); it != items.end(); it++) {
    itemNames.push_back((*it)->text().toStdString());
  }
  return itemNames;
}

/** Returns a list of names of currently selected parameters
* @return :: parameter names
*/
std::vector<std::string> QtReflSaveTabView::getSelectedParameters() const {
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
  for (auto it = names.begin(); it != names.end(); it++) {
    m_ui.listOfWorkspaces->addItem(QString::fromStdString(*it));
  }
}

/** Set the 'List of logged parameters' widget with workspace run logs
* @param logs :: The list of workspace run logs
*/
void QtReflSaveTabView::setParametersList(
    const std::vector<std::string> &logs) const {
  for (auto it = logs.begin(); it != logs.end(); it++) {
    m_ui.listOfLoggedParameters->addItem(QString::fromStdString(*it));
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

} // namespace CustomInterfaces
} // namespace Mantid
