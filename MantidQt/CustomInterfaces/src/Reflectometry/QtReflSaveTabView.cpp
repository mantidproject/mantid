#include "MantidQtCustomInterfaces/Reflectometry/QtReflSaveTabView.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflSaveTabPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {

/** Constructor
* @param parent :: The parent of this view
*/
QtReflSaveTabView::QtReflSaveTabView(QWidget *parent) : m_presenter() {

  UNUSED_ARG(parent);
  initLayout();

  m_presenter.reset(new ReflSaveTabPresenter(this));
  m_presenter->notify(IReflSaveTabPresenter::populateWorkspaceListFlag);
}

/** Destructor
*/
QtReflSaveTabView::~QtReflSaveTabView() {}

/**
Initialize the Interface
*/
void QtReflSaveTabView::initLayout() { m_ui.setupUi(this); }

/** Returns the save path
* @return :: The save path
*/
std::string QtReflSaveTabView::getSavePath() const { 
  return m_ui.savePathEdit->text().toStdString(); 
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
* @return :: The reg exp check
*/
bool QtReflSaveTabView::getRegExpCheck() const {
  return m_ui.regExpCheckBox->isChecked();
}

/** Returns the name of an item at index in 'List of workspaces' widget
* @return :: item name
*/
std::string QtReflSaveTabView::getListOfWorkspacesItem(int index) const {
  return m_ui.listOfWorkspaces->item(index)->text().toStdString();
}

/** Returns the name of an item at index in 'List of logged parameters' widget
* @return :: item name
*/
std::string QtReflSaveTabView::getListOfParametersItem(int index) const {
  return m_ui.listOfLoggedParameters->item(index)->text().toStdString();
}

/** Returns the spectra list as a single string
* @return :: Spectra list
*/
std::string QtReflSaveTabView::getSpectraList() const {
  return m_ui.spectraListEdit->text().toStdString();
}

/** Returns the file format
* @return :: File format
*/
std::string QtReflSaveTabView::getFileFormat() const {
  return m_ui.fileFormatComboBox->currentText().toStdString();
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
  return m_ui.separatorButtonGroup->checkedButton()->text().toStdString();
}

/** Clear the 'List of workspaces' widget
*/
void QtReflSaveTabView::clearWorkspaceList() const {
  m_ui.listOfWorkspaces->clear();
}

/** Set the 'List of workspaces' text field with workspace names
* @param names :: The list of workspace names
*/
void QtReflSaveTabView::setWorkspaceList(
    const std::vector<std::string> &names) const {
  for (auto it = names.begin(); it != names.end(); it++) {
    m_ui.listOfWorkspaces->addItem(QString::fromStdString(*it));
  }
}

} // namespace CustomInterfaces
} // namespace Mantid
