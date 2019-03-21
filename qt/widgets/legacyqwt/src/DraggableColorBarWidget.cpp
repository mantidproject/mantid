// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/LegacyQwt/DraggableColorBarWidget.h"
#include "MantidQtWidgets/LegacyQwt/MantidColorMap.h"
#include "MantidQtWidgets/LegacyQwt/PowerScaleEngine.h"

#include "MantidQtWidgets/Common/DoubleSpinBox.h"
#include "MantidQtWidgets/Common/GraphOptions.h"
#include "MantidQtWidgets/Common/TSVSerialiser.h"

#include <QApplication>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <qwt_scale_widget.h>

namespace MantidQt {
namespace MantidWidgets {
/**
 * Constructor giving a colorbar
 * @param parent A parent widget
 * @param minPositiveValue A minimum positive value for the Log10 scale
 */
DraggableColorBarWidget::DraggableColorBarWidget(QWidget *parent,
                                                 const double &minPositiveValue)
    : QFrame(parent), m_minPositiveValue(minPositiveValue), m_dragging(false),
      m_y(0), m_dtype(), m_nth_power(2.0) {
  m_scaleWidget = new QwtScaleWidget(QwtScaleDraw::RightScale);
  m_scaleWidget->setColorBarEnabled(true);
  m_scaleWidget->setColorBarWidth(20);
  m_scaleWidget->setAlignment(QwtScaleDraw::RightScale);
  m_scaleWidget->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
  m_scaleWidget->setCursor(Qt::OpenHandCursor);

  m_minValueBox = new QLineEdit();
  m_maxValueBox = new QLineEdit();
  m_minValueBox->setMinimumWidth(40);
  m_maxValueBox->setMinimumWidth(40);
  m_minValueBox->setMaximumWidth(60);
  m_maxValueBox->setMaximumWidth(60);
  m_minValueBox->setValidator(new QDoubleValidator(m_minValueBox));
  m_maxValueBox->setValidator(new QDoubleValidator(m_maxValueBox));
  // Ensure the boxes start empty, this is important for checking if values have
  // been set from the scripting side
  m_minValueBox->setText("");
  m_maxValueBox->setText("");
  connect(m_minValueBox, SIGNAL(editingFinished()), this,
          SLOT(minValueChanged()));
  connect(m_maxValueBox, SIGNAL(editingFinished()), this,
          SLOT(maxValueChanged()));

  QVBoxLayout *lColormapLayout = new QVBoxLayout;
  lColormapLayout->addWidget(m_maxValueBox);
  lColormapLayout->addWidget(m_scaleWidget);
  lColormapLayout->addWidget(m_minValueBox);

  m_scaleOptions = new QComboBox;
  m_scaleOptions->addItem("Log10", QVariant(GraphOptions::Log10));
  m_scaleOptions->addItem("Linear", QVariant(GraphOptions::Linear));
  m_scaleOptions->addItem("Power", QVariant(GraphOptions::Power));
  m_scaleOptions->setCurrentIndex(1); // linear default
  connect(m_scaleOptions, SIGNAL(currentIndexChanged(int)), this,
          SLOT(scaleOptionsChanged(int)));

  // Controls for exponent for power scale type
  m_lblN = new QLabel(tr("n ="));
  m_lblN->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
  m_dspnN = new DoubleSpinBox();
  m_dspnN->setValue(m_nth_power);
  connect(m_dspnN, SIGNAL(valueChanged(double)), this,
          SLOT(nPowerChanged(double)));

  QGridLayout *options_layout = new QGridLayout;
  options_layout->addWidget(m_scaleOptions, 1, 0, 1, 2);
  options_layout->addWidget(m_lblN, 2, 0);
  options_layout->addWidget(m_dspnN, 2, 1);
  options_layout->setRowStretch(0, 4);
  options_layout->setRowStretch(1, 1);
  options_layout->setRowStretch(2, 1);

  QHBoxLayout *colourmap_layout = new QHBoxLayout;
  colourmap_layout->addLayout(lColormapLayout);
  colourmap_layout->addLayout(options_layout);
  this->setLayout(colourmap_layout);
}

void DraggableColorBarWidget::scaleOptionsChanged(int i) {
  if (m_scaleOptions->itemData(i).toUInt() == 2) {
    m_dspnN->setEnabled(true);
  } else
    m_dspnN->setEnabled(false);

  emit scaleTypeChanged(m_scaleOptions->itemData(i).toUInt());
}

void DraggableColorBarWidget::nPowerChanged(double nth_power) {
  emit nthPowerChanged(nth_power);
}

/**
 * Set up a new colour map.
 * @param colorMap :: Reference to the new colour map.
 */
void DraggableColorBarWidget::setupColorBarScaling(
    const MantidColorMap &colorMap) {
  double minValue = m_minValueBox->displayText().toDouble();
  double maxValue = m_maxValueBox->displayText().toDouble();

  auto type = colorMap.getScaleType();
  if (type == MantidColorMap::ScaleType::Linear) {
    QwtLinearScaleEngine linScaler;
    m_scaleWidget->setScaleDiv(
        linScaler.transformation(),
        linScaler.divideScale(minValue, maxValue, 20, 5));
    m_scaleWidget->setColorMap(QwtDoubleInterval(minValue, maxValue), colorMap);
  } else if (type == MantidColorMap::ScaleType::Power) {
    PowerScaleEngine powerScaler;
    m_scaleWidget->setScaleDiv(
        powerScaler.transformation(),
        powerScaler.divideScale(minValue, maxValue, 20, 5));
    m_scaleWidget->setColorMap(QwtDoubleInterval(minValue, maxValue), colorMap);
  } else {
    QwtLog10ScaleEngine logScaler;
    double logmin(minValue);
    if (logmin <= 0.0) {
      logmin = m_minPositiveValue;
      m_minValueBox->blockSignals(true);
      setMinValue(logmin);
      m_minValueBox->blockSignals(false);
    }
    if (maxValue <= 0) {
      maxValue = 10.;
      m_maxValueBox->blockSignals(true);
      setMaxValue(maxValue);
      m_maxValueBox->blockSignals(false);
    }
    m_scaleWidget->setScaleDiv(logScaler.transformation(),
                               logScaler.divideScale(logmin, maxValue, 20, 5));
    m_scaleWidget->setColorMap(QwtDoubleInterval(logmin, maxValue), colorMap);
  }
  m_scaleOptions->blockSignals(true);
  m_scaleOptions->setCurrentIndex(
      m_scaleOptions->findData(static_cast<int>(type)));
  if (m_scaleOptions->findData(static_cast<int>(type)) == 2) {
    m_dspnN->setEnabled(true);
  } else {
    m_dspnN->setEnabled(false);
  }
  m_scaleOptions->blockSignals(false);
}

/// Send the minValueChanged signal
void DraggableColorBarWidget::minValueChanged() {
  const auto value = m_minValueBox->text().toDouble();
  emit minValueEdited(value);
  emit minValueChanged(value);
}

/// Send the maxValueChanged signal
void DraggableColorBarWidget::maxValueChanged() {
  const auto value = m_maxValueBox->text().toDouble();
  emit maxValueEdited(value);
  emit maxValueChanged(value);
}

/**
 * Update the minimum and maximum range of the scale
 * @param vmin New minimum of the scale
 * @param vmax New maximum of the scale
 */

void DraggableColorBarWidget::setClim(double vmin, double vmax) {
  setMinValue(vmin);
  setMaxValue(vmax);
}

/**
 * Set a new min value and update the widget.
 * @param value :: The new value
 */
void DraggableColorBarWidget::setMinValue(double value) {
  setMinValueText(value);
  updateScale();
  if (!m_minValueBox->signalsBlocked()) {
    minValueChanged();
  }
}

/**
 * Set a new max value and update the widget.
 * @param value :: The new value
 */
void DraggableColorBarWidget::setMaxValue(double value) {
  setMaxValueText(value);
  updateScale();
  if (!m_maxValueBox->signalsBlocked()) {
    maxValueChanged();
  }
}

/**
 * returns the min value as QString
 */
QString DraggableColorBarWidget::getMinValue() const {
  return m_minValueBox->text();
}

/**
 * returns the min value as QString
 */
QString DraggableColorBarWidget::getMaxValue() const {
  return m_maxValueBox->text();
}

/**
 * returns the mnth powder as QString
 */
QString DraggableColorBarWidget::getNthPower() const { return m_dspnN->text(); }

/**
 * Update the min value text box.
 * @param value :: Value to be displayed in the text box.
 */
void DraggableColorBarWidget::setMinValueText(double value) {
  m_minValueBox->setText(QString::number(value));
}

/**
 * Update the max value text box.
 * @param value :: Value to be displayed in the text box.
 */
void DraggableColorBarWidget::setMaxValueText(double value) {
  m_maxValueBox->setText(QString::number(value));
}

/**
 * Set the minimum positive value for use with the Log10 scale. Values below
 * this will not be displayed on a Log10 scale.
 */
void DraggableColorBarWidget::setMinPositiveValue(double value) {
  m_minPositiveValue = value;
}

/**
 * Return the scale type: Log10 or Linear.
 */
int DraggableColorBarWidget::getScaleType() const {
  return m_scaleOptions->itemData(m_scaleOptions->currentIndex()).toUInt();
}

/**
 * Set the scale type: Log10 or Linear.
 */
void DraggableColorBarWidget::setScaleType(int type) {
  m_scaleOptions->setCurrentIndex(m_scaleOptions->findData(type));
}

void DraggableColorBarWidget::setNthPower(double nth_power) {
  m_dspnN->setValue(nth_power);
}

/**
 * Update the colour scale after the range changes.
 */
void DraggableColorBarWidget::updateScale() {
  double minValue = m_minValueBox->displayText().toDouble();
  double maxValue = m_maxValueBox->displayText().toDouble();
  GraphOptions::ScaleType type = (GraphOptions::ScaleType)m_scaleOptions
                                     ->itemData(m_scaleOptions->currentIndex())
                                     .toUInt();
  if (type == GraphOptions::Linear) {
    QwtLinearScaleEngine linScaler;
    m_scaleWidget->setScaleDiv(
        linScaler.transformation(),
        linScaler.divideScale(minValue, maxValue, 20, 5));
  } else if (type == GraphOptions::Power) {
    PowerScaleEngine powerScaler;
    m_scaleWidget->setScaleDiv(
        powerScaler.transformation(),
        powerScaler.divideScale(minValue, maxValue, 20, 5));
  } else {
    QwtLog10ScaleEngine logScaler;
    double logmin(minValue);
    if (logmin <= 0.0) {
      logmin = m_minPositiveValue;
    }
    m_scaleWidget->setScaleDiv(logScaler.transformation(),
                               logScaler.divideScale(logmin, maxValue, 20, 5));
  }
}

/**
 * Respond to a mouse press event. Start dragging to modify the range (min or
 * max value).
 */
void DraggableColorBarWidget::mousePressEvent(QMouseEvent *e) {
  QRect rect = m_scaleWidget->rect();
  if (e->x() > rect.left() && e->x() < rect.right()) {
    m_dragging = true;
    m_y = e->y();
    m_dtype = (m_y > height() / 2) ? Bottom : Top;
    QApplication::setOverrideCursor(Qt::ClosedHandCursor);
  }
}

/**
 * Respond to mouse move event. If the left button is down change the min or
 * max.
 */
void DraggableColorBarWidget::mouseMoveEvent(QMouseEvent *e) {
  if (!m_dragging)
    return;

  double minValue = m_minValueBox->displayText().toDouble();
  double maxValue = m_maxValueBox->displayText().toDouble();

  if (m_dtype == Bottom) {
    minValue += double(e->y() - m_y) / height() * (maxValue - minValue);
    setMinValueText(minValue);
  } else {
    maxValue += double(e->y() - m_y) / height() * (maxValue - minValue);
    setMaxValueText(maxValue);
  }
  m_y = e->y();
  updateScale();
}

/**
 * Respond to a mouse release event. Finish all dragging.
 */
void DraggableColorBarWidget::mouseReleaseEvent(QMouseEvent * /*e*/) {
  if (!m_dragging)
    return;
  if (m_dtype == Bottom) {
    minValueChanged();
  } else {
    maxValueChanged();
  }
  QApplication::restoreOverrideCursor();
  m_dragging = false;
}

/**
 * Save the state of the color map widget to a project file.
 * @return string representing the current state of the color map widget.
 */
std::string DraggableColorBarWidget::saveToProject() const {
  API::TSVSerialiser tsv;
  tsv.writeLine("ScaleType") << getScaleType();
  tsv.writeLine("Power") << getNthPower();
  tsv.writeLine("MinValue") << getMinValue();
  tsv.writeLine("MaxValue") << getMaxValue();
  return tsv.outputLines();
}

/**
 * Load the state of the color map widget from a project file.
 * @param lines :: string representing the current state of the color map
 * widget.
 */
void DraggableColorBarWidget::loadFromProject(const std::string &lines) {
  API::TSVSerialiser tsv(lines);

  int scaleType;
  double min, max, power;
  tsv.selectLine("ScaleType");
  tsv >> scaleType;
  tsv.selectLine("Power");
  tsv >> power;
  tsv.selectLine("MinValue");
  tsv >> min;
  tsv.selectLine("MaxValue");
  tsv >> max;

  setScaleType(scaleType);
  setNthPower(power);
  setMinValue(min);
  setMaxValue(max);
}
} // namespace MantidWidgets
} // namespace MantidQt
