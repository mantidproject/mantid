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

/** Returns the list of workspaces as a single string
* @return :: List of workspaces
*/
std::string QtReflSaveTabView::getListOfWorkspaces() const {
  return m_ui.listOfWorkspacesEdit->toPlainText().toStdString();
}

/** Returns the list of logged parameters as a single string
* @return :: List of parameters
*/
std::string QtReflSaveTabView::getListOfParameters() const {
  return m_ui.listOfLoggedParametersEdit->toPlainText().toStdString();
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

} // namespace CustomInterfaces
} // namespace Mantid
