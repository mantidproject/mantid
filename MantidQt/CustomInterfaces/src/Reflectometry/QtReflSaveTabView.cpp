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

} // namespace CustomInterfaces
} // namespace Mantid
