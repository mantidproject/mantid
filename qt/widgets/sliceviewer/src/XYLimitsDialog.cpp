#include "MantidQtWidgets/SliceViewer/XYLimitsDialog.h"
#include "MantidKernel/UnitLabel.h"
#include "MantidQtWidgets/Common/QStringUtils.h"
#include <QIntValidator>

using MantidQt::API::toQStringInternal;

XYLimitsDialog::XYLimitsDialog(QWidget *parent) : QDialog(parent) {
  ui.setupUi(this);
  ui.textXMax->setValidator(new QDoubleValidator(ui.textXMax));
  ui.textYMax->setValidator(new QDoubleValidator(ui.textYMax));
  ui.textXMin->setValidator(new QDoubleValidator(ui.textXMin));
  ui.textYMin->setValidator(new QDoubleValidator(ui.textYMin));

  QObject::connect(ui.btnOK, SIGNAL(clicked()), this, SLOT(accept()));
  QObject::connect(ui.btnCancel, SIGNAL(clicked()), this, SLOT(reject()));
}

XYLimitsDialog::~XYLimitsDialog() {}

//------------------------------------------------------------------------------------------
/** Set the labels for the X dimensions
 * @param dim : IMDDimension */
void XYLimitsDialog::setXDim(Mantid::Geometry::IMDDimension_const_sptr dim) {
  ui.lblXName->setText(QString::fromStdString(dim->getName()));
  ui.lblXUnits->setText(toQStringInternal(dim->getUnits().utf8()));
}

/** Set the labels for the Y dimensions
 * @param dim : IMDDimension */
void XYLimitsDialog::setYDim(Mantid::Geometry::IMDDimension_const_sptr dim) {
  ui.lblYName->setText(QString::fromStdString(dim->getName()));
  ui.lblYUnits->setText(toQStringInternal(dim->getUnits().utf8()));
}

//------------------------------------------------------------------------------------------
/** Set the min/max value externally
 *
 * @param x0 :: x min
 * @param x1 :: x max
 * @param y0 :: y min
 * @param y1 :: y max
 */
void XYLimitsDialog::setLimits(double x0, double x1, double y0, double y1) {
  ui.textXMin->setText(QString::number(x0));
  ui.textXMax->setText(QString::number(x1));
  ui.textYMin->setText(QString::number(y0));
  ui.textYMax->setText(QString::number(y1));
}

//------------------------------------------------------------------------------------------
/// @return the X min value in the gui
double XYLimitsDialog::getXMin() { return ui.textXMin->text().toDouble(); }

/// @return the X Max  value in the gui
double XYLimitsDialog::getXMax() { return ui.textXMax->text().toDouble(); }

/// @return the X min value in the gui
double XYLimitsDialog::getYMin() { return ui.textYMin->text().toDouble(); }

/// @return the X Max  value in the gui
double XYLimitsDialog::getYMax() { return ui.textYMax->text().toDouble(); }
