#include "MantidQtWidgets/SliceViewer/DimensionSliceWidget.h"
#include "MantidKernel/UnitLabel.h"
#include "MantidQtWidgets/Common/QStringUtils.h"
#include <QLayout>
#include <iosfwd>

namespace MantidQt {
using API::toQStringInternal;
namespace SliceViewer {

DimensionSliceWidget::DimensionSliceWidget(QWidget *parent)
    : QWidget(parent), m_dim(), m_dimIndex(0), m_shownDim(0), m_slicePoint(0.0),
      m_showRebinControls(false) {
  ui.setupUi(this);

  QObject::connect(ui.horizontalSlider, SIGNAL(valueChanged(double)), this,
                   SLOT(sliderMoved()));
  QObject::connect(ui.doubleSpinBox, SIGNAL(editingFinished()), this,
                   SLOT(spinBoxChanged()));
  QObject::connect(ui.spinThickness, SIGNAL(editingFinished()), this,
                   SLOT(spinThicknessChanged()));
  QObject::connect(ui.spinBins, SIGNAL(editingFinished()), this,
                   SLOT(spinBinsChanged()));
  QObject::connect(ui.btnX, SIGNAL(toggled(bool)), this, SLOT(btnXYChanged()));
  QObject::connect(ui.btnY, SIGNAL(toggled(bool)), this, SLOT(btnXYChanged()));

  // Hide the rebinning controls
  ui.spinBins->setVisible(false);
  ui.spinThickness->setVisible(false);
  ui.lblRebinInfo->setVisible(false);
}

DimensionSliceWidget::~DimensionSliceWidget() {}

//-------------------------------------------------------------------------------------------------
/** Slot called when the slider moves */
void DimensionSliceWidget::sliderMoved() {
  // Find the slice point
  m_slicePoint = ui.horizontalSlider->value();

  // This will, in turn, emit the changedSlicePoint() signal
  ui.doubleSpinBox->setValue(m_slicePoint);
  spinBoxChanged();
}

//-------------------------------------------------------------------------------------------------
/** Slot called when the slider moves */
void DimensionSliceWidget::spinBoxChanged() {
  // This is the slice point
  m_slicePoint = ui.doubleSpinBox->value();

  // Set the slider to the matching point
  ui.horizontalSlider->blockSignals(true);
  ui.horizontalSlider->setValue(m_slicePoint);
  ui.horizontalSlider->blockSignals(false);

  // Emit that the user changed the slicing point
  emit changedSlicePoint(m_dimIndex, m_slicePoint);
}

//-------------------------------------------------------------------------------------------------
/** Called when the X button is clicked */
void DimensionSliceWidget::btnXYChanged() {
  int oldDim = m_shownDim;
  ui.btnX->blockSignals(true);
  ui.btnY->blockSignals(true);
  if (ui.btnX->isChecked() && ui.btnY->isChecked()) {
    // Toggle when both are checked
    if (m_shownDim == 0)
      this->setShownDim(1);
    else
      this->setShownDim(0);
  } else if (ui.btnX->isChecked())
    this->setShownDim(0);
  else if (ui.btnY->isChecked())
    this->setShownDim(1);
  else
    this->setShownDim(-1);

  // Emit that the user changed the shown dimension
  emit changedShownDim(m_dimIndex, m_shownDim, oldDim);

  ui.btnX->blockSignals(false);
  ui.btnY->blockSignals(false);
}

//-------------------------------------------------------------------------------------------------
/** Set the shown dimension
 *
 * @param dim :: -1 = None, 0 = X, 1 = Y. 2+ reserved for higher dimensions
 */
void DimensionSliceWidget::setShownDim(int dim) {
  m_shownDim = dim;
  ui.btnX->blockSignals(true);
  ui.btnY->blockSignals(true);
  ui.btnX->setChecked(m_shownDim == 0);
  ui.btnY->setChecked(m_shownDim == 1);
  ui.btnX->blockSignals(false);
  ui.btnY->blockSignals(false);
  /// Slice if dimension is not X or Y AND is not integrated
  bool slicing = (m_shownDim == -1 && !m_dim->getIsIntegrated());

  ui.horizontalSlider->setVisible(slicing);
  ui.doubleSpinBox->setVisible(slicing);
  ui.lblUnits->setVisible(slicing);

  // Make the spacer expand to keep the buttons in the same spot
  if (slicing) {
    if (ui.horizontalSpacer != nullptr) {
      // Remove the 3rd item (if it's not gone already) = the spacer
      ui.horizontalLayout->removeItem(ui.horizontalLayout->itemAt(3));
      delete ui.horizontalSpacer;
      ui.horizontalSpacer = nullptr;
    }
  } else {
    // Put the spacer back, if needed
    if (ui.horizontalSpacer == nullptr) {
      ui.horizontalSpacer =
          new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);
      ui.horizontalLayout->insertSpacerItem(3, ui.horizontalSpacer);
    }
  }

  // Hide or show the rebinning controls
  ui.spinBins->setVisible(m_showRebinControls && !slicing);
  ui.spinThickness->setVisible(m_showRebinControls && slicing);
  ui.lblRebinInfo->setVisible(m_showRebinControls);
  if (slicing)
    ui.lblRebinInfo->setText("thick");
  else
    ui.lblRebinInfo->setText("bins");

  this->update();
}

//-------------------------------------------------------------------------------------------------
/** Sets the slice point value.
 *
 * @param value :: where to slice
 */
void DimensionSliceWidget::setSlicePoint(double value) {
  // This will trigger the required events
  ui.horizontalSlider->setValue(value);
}

//-------------------------------------------------------------------------------------------------
/** Sets the min/max to show on the widget
 *
 * @param min :: min value
 * @param max :: max value
 */
void DimensionSliceWidget::setMinMax(double min, double max) {
  if (!m_dim)
    return;
  ui.lblName->setText(QString::fromStdString(m_dim->getName()));
  ui.lblUnits->setText(toQStringInternal(m_dim->getUnits().utf8()));

  ui.horizontalSlider->setRange(min, max, m_dim->getBinWidth());

  ui.doubleSpinBox->setMinimum(min);
  ui.doubleSpinBox->setMaximum(max);
  ui.doubleSpinBox->setSingleStep(m_dim->getBinWidth());

  // Make sure the slice point is in range
  if (m_slicePoint < min)
    m_slicePoint = min;
  if (m_slicePoint > max)
    m_slicePoint = max;
  ui.doubleSpinBox->setValue(m_slicePoint);
}

//-------------------------------------------------------------------------------------------------
/** Set the dimension to display */
void DimensionSliceWidget::setDimension(
    int index, Mantid::Geometry::IMDDimension_const_sptr dim) {
  m_dim = dim;
  m_dimIndex = index;
  // set the limits of the slider to be the bin centres and not
  // the edges of the bins
  double half_bin_width = m_dim->getBinWidth() * 0.5;
  double min = m_dim->getMinimum() + half_bin_width;
  double max = m_dim->getMaximum() - half_bin_width;
  this->setMinMax(min, max);
}

//-------------------------------------------------------------------------------------------------
/** Sets whether to display the rebinning controls */
void DimensionSliceWidget::showRebinControls(bool show) {
  m_showRebinControls = show;
  this->setShownDim(m_shownDim);
}

/** @return whether the rebinning controls are shown */
bool DimensionSliceWidget::showRebinControls() const {
  return m_showRebinControls;
}

//-------------------------------------------------------------------------------------------------
/** @return the number of bins to rebin to */
int DimensionSliceWidget::getNumBins() const { return ui.spinBins->value(); }

/** Sets the number of bins to rebin to */
void DimensionSliceWidget::setNumBins(int val) {
  ui.spinBins->setValue(val);
  spinBinsChanged();
}

//-------------------------------------------------------------------------------------------------
/** @return the thickness to integrate when rebinning */
double DimensionSliceWidget::getThickness() const {
  return ui.spinThickness->value();
}

/** Sets the thickness to integrate when rebinning */
void DimensionSliceWidget::setThickness(double val) {
  ui.spinThickness->setValue(val);
  spinThicknessChanged();
}

//-------------------------------------------------------------------------------------------------
/** Slot called when the thickness to rebin to is changed */
void DimensionSliceWidget::spinThicknessChanged() {
  emit changedThickness(m_dimIndex, this->getThickness());
}

//-------------------------------------------------------------------------------------------------
/** Slot called when the number of bins to rebin to is changed */
void DimensionSliceWidget::spinBinsChanged() {
  emit changedNumBins(m_dimIndex, this->getNumBins());
}
} // namespace SliceViewer
} // namespace MantidQt
