#include "MantidQtCustomInterfaces/Reflectometry/QtReflEventTabView.h"
#include "MantidQtCustomInterfaces/Reflectometry/QtReflEventView.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflEventTabPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {

//----------------------------------------------------------------------------------------------
/** Constructor
* @param parent :: [input] The parent of this widget
*/
QtReflEventTabView::QtReflEventTabView(QWidget *parent) {

  UNUSED_ARG(parent);
  initLayout();
}

//----------------------------------------------------------------------------------------------
/** Destructor
*/
QtReflEventTabView::~QtReflEventTabView() {}

/**
Initialise the interface
*/
void QtReflEventTabView::initLayout() {
  m_ui.setupUi(this);

  QtReflEventView *event_1 = new QtReflEventView(this);
  m_ui.toolbox->addItem(event_1, "Group 1");

  QtReflEventView *event_2 = new QtReflEventView(this);
  m_ui.toolbox->addItem(event_2, "Group 2");

  std::vector<IReflEventPresenter *> presenters;
  presenters.push_back(event_1->getPresenter());
  presenters.push_back(event_2->getPresenter());

  m_presenter.reset(new ReflEventTabPresenter(presenters));
}

/** Returns the presenter managing this view
* @return :: A pointer to the presenter
*/
IReflEventTabPresenter *QtReflEventTabView::getPresenter() const {

  return m_presenter.get();
}

} // namespace CustomInterfaces
} // namespace MantidQt
