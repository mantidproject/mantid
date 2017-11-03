#include "MantidVatesSimpleGuiQtWidgets/RotationPointDialog.h"

#include <QDoubleValidator>

namespace Mantid {
namespace Vates {
namespace SimpleGui {

/**
 * The class constructor. This sets up the UI, adds validators to the
 * line editors and connects the signal/slot for transmitting the coordinates.
 */
RotationPointDialog::RotationPointDialog(QWidget *parent) : QDialog(parent) {
  this->ui.setupUi(this);
  this->ui.xLineEdit->setValidator(new QDoubleValidator(this));
  this->ui.yLineEdit->setValidator(new QDoubleValidator(this));
  this->ui.zLineEdit->setValidator(new QDoubleValidator(this));

  // Gather the coordinates
  QObject::connect(this->ui.buttonBox, SIGNAL(accepted()), this,
                   SLOT(getCoordinates()));
#ifdef __APPLE__
  // On macOS the dialogs appear behind everything by default. Need to find
  // a better fix that this...
  setModal(true);
#endif
}

RotationPointDialog::~RotationPointDialog() {}

/**
 * This function retrieves the individual coordinates from the line editors
 * and transmits that information via a signal.
 */
void RotationPointDialog::getCoordinates() {
  double x = this->ui.xLineEdit->text().toDouble();
  double y = this->ui.yLineEdit->text().toDouble();
  double z = this->ui.zLineEdit->text().toDouble();
  emit this->sendCoordinates(x, y, z);
}
}
}
}
