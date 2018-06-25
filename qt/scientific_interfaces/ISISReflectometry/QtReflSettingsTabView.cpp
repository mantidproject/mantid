#include "QtReflSettingsTabView.h"
#include "QtReflSettingsView.h"
#include "ReflSettingsTabPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {

/** Constructor
* @param parent :: [input] The parent of this widget
*/
QtReflSettingsTabView::QtReflSettingsTabView(QWidget *parent) {

  UNUSED_ARG(parent);
  initLayout();
}

void QtReflSettingsTabView::subscribe(IReflSettingsTabPresenter* notifyee) {
  m_notifyee = notifyee;
}

/**
Initialise the interface
*/
void QtReflSettingsTabView::initLayout() {
  m_ui.setupUi(this);

  QtReflSettingsView *settings_1 = new QtReflSettingsView(0, this);
  m_ui.toolbox->addItem(settings_1, "Group 1");

  QtReflSettingsView *settings_2 = new QtReflSettingsView(1, this);
  m_ui.toolbox->addItem(settings_2, "Group 2");
}

} // namespace CustomInterfaces
} // namespace Mantid
