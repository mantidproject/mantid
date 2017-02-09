#include "MantidVatesSimpleGuiViewWidgets/ColorMapEditorPanel.h"
#include <pqApplicationCore.h>
#include <QWidget>
#include <QDialog>

namespace Mantid {
namespace Vates {
namespace SimpleGui {

ColorMapEditorPanel::ColorMapEditorPanel(QWidget *parent) : QDialog(parent) {}

ColorMapEditorPanel::~ColorMapEditorPanel() {}
/**
 * Set up the connections for the pop up window
 */
void ColorMapEditorPanel::setUpPanel() {
  this->ui.setupUi(this);
  this->setWindowTitle("Color Editor Panel");
  this->hide();
  this->ui.dockWidget->installEventFilter(this);
  pqApplicationCore::instance()->registerManager("COLOR_EDITOR_PANEL",
                                                 this->ui.dockWidget);

  QObject::connect(this, SIGNAL(showPopUpWindow()), this,
                   SLOT(onShowPopUpWindow()), Qt::QueuedConnection);

  QObject::connect(this, SIGNAL(hidePopUpWindow()), this,
                   SLOT(onHidePopUpWindow()), Qt::QueuedConnection);

#ifdef __APPLE__
  // On macOS the dialogs appear behind everything by default. Need to find
  // a better fix that this...
  setModal(true);
#endif
}

/**
 * Show the pop up window
 */
void ColorMapEditorPanel::onShowPopUpWindow() {
  this->show();
  this->raise();

  this->ui.dockWidget->show();
  this->ui.dockWidget->raise();
}

/**
 * Hide the pop up window
 */
void ColorMapEditorPanel::onHidePopUpWindow() {
  this->hide();
  this->ui.dockWidget->hide();
}

/**
 * This function listens to visibility changes of the ColorMapEditor widget
 * and acts on them
 * @param obj the subject of the event
 * @param ev the actual event
 * @return true if the event was handled
 */
bool ColorMapEditorPanel::eventFilter(QObject *obj, QEvent *ev) {
  if (this->ui.dockWidget == obj && ev->type() == QEvent::ShowToParent) {
    emit showPopUpWindow();
    return true;
  } else if (this->ui.dockWidget == obj && ev->type() == QEvent::Hide) {
    // Note that we actively need to hide the dockWidget, or else
    // it will not be set visible the next time!
    emit hidePopUpWindow();
    return true;
  }
  return QDialog::eventFilter(obj, ev);
}
}
}
}
