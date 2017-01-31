#include "MantidQtCustomInterfaces/Reflectometry/QtReflEventView.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflEventPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {

//----------------------------------------------------------------------------------------------
/** Constructor
* @param parent :: [input] The parent of this widget
*/
QtReflEventView::QtReflEventView(QWidget *parent) {

  UNUSED_ARG(parent);
  initLayout();

  m_presenter.reset(new ReflEventPresenter(this));
}

//----------------------------------------------------------------------------------------------
/** Destructor
*/
QtReflEventView::~QtReflEventView() {}

/**
Initialise the Interface
*/
void QtReflEventView::initLayout() { m_ui.setupUi(this); }

/** Returns the presenter managing this view
* @return :: A pointer to the presenter
*/
IReflEventPresenter *QtReflEventView::getPresenter() const {

  return m_presenter.get();
}

/** Returns the number of time slices
* @return :: The number of time slices
*/
std::string QtReflEventView::getTimeSlices() const {

  return m_ui.evenTimeSlicesEdit->text().toStdString();
}

} // namespace CustomInterfaces
} // namespace Mantid
