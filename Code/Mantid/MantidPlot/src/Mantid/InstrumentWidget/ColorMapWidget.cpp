#include "ColorMapWidget.h"
#include "MantidQtAPI/MantidColorMap.h"
#include "MantidQtAPI/GraphOptions.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QMouseEvent>
#include <QApplication>
#include <qwt_scale_widget.h>
#include <qwt_scale_engine.h>

/**
  * Constructor.
  * @param type The scale type, e.g. "Linear" or "Log10"
  * @param parent A parent widget
  * @param minPositiveValue A minimum positive value for the Log10 scale
  */
ColorMapWidget::ColorMapWidget(int type,QWidget* parent,const double& minPositiveValue):
QFrame(parent),m_minPositiveValue(minPositiveValue),m_dragging(false)
{
  m_scaleWidget = new QwtScaleWidget(QwtScaleDraw::RightScale);
  m_scaleWidget->setColorBarEnabled(true);
  m_scaleWidget->setColorBarWidth(20);
  m_scaleWidget->setAlignment(QwtScaleDraw::RightScale);
  m_scaleWidget->setLabelAlignment( Qt::AlignRight | Qt::AlignVCenter);
  m_scaleWidget->setCursor(Qt::OpenHandCursor);

  m_minValueBox = new QLineEdit();
  m_maxValueBox = new QLineEdit();
  m_minValueBox->setMinimumWidth(40);
  m_maxValueBox->setMinimumWidth(40);
  m_minValueBox->setMaximumWidth(60);
  m_maxValueBox->setMaximumWidth(60);
  m_minValueBox->setValidator(new QDoubleValidator(m_minValueBox));
  m_maxValueBox->setValidator(new QDoubleValidator(m_maxValueBox));
  //Ensure the boxes start empty, this is important for checking if values have been set from the scripting side
  m_minValueBox->setText("");
  m_maxValueBox->setText("");
  connect(m_minValueBox,SIGNAL(editingFinished()),this,SLOT(minValueChanged()));
  connect(m_maxValueBox,SIGNAL(editingFinished()),this,SLOT(maxValueChanged()));

  QVBoxLayout* lColormapLayout = new QVBoxLayout;
  lColormapLayout->addWidget(m_maxValueBox);
  lColormapLayout->addWidget(m_scaleWidget);
  lColormapLayout->addWidget(m_minValueBox);

  m_scaleOptions = new QComboBox;
  m_scaleOptions->addItem("Log10", QVariant(GraphOptions::Log10));
  m_scaleOptions->addItem("Linear", QVariant(GraphOptions::Linear));
  m_scaleOptions->setCurrentIndex(m_scaleOptions->findData(type));
  connect(m_scaleOptions, SIGNAL(currentIndexChanged(int)), this, SLOT(scaleOptionsChanged(int)));

  QVBoxLayout* options_layout = new QVBoxLayout;
  options_layout->addStretch();
  options_layout->addWidget(m_scaleOptions);

  QHBoxLayout *colourmap_layout = new QHBoxLayout;
  colourmap_layout->addLayout(lColormapLayout);
  colourmap_layout->addLayout(options_layout);
  this->setLayout(colourmap_layout);

}

void ColorMapWidget::scaleOptionsChanged(int i)
{
  emit scaleTypeChanged(m_scaleOptions->itemData(i).toUInt());
}

/**
 * Set up a new colour map.
 * @param colorMap :: Reference to the new colour map.
 */
void ColorMapWidget::setupColorBarScaling(const MantidColorMap& colorMap)
{
  double minValue = m_minValueBox->displayText().toDouble();
  double maxValue = m_maxValueBox->displayText().toDouble();

  GraphOptions::ScaleType type = colorMap.getScaleType();
  if( type == GraphOptions::Linear )
  {
    QwtLinearScaleEngine linScaler;
    m_scaleWidget->setScaleDiv(linScaler.transformation(), linScaler.divideScale(minValue, maxValue,  20, 5));
    m_scaleWidget->setColorMap(QwtDoubleInterval(minValue, maxValue),colorMap);
  }
  else
 {
    QwtLog10ScaleEngine logScaler;    
    double logmin(minValue);
    if( logmin <= 0.0 )
    {
      logmin = m_minPositiveValue;
      m_minValueBox->blockSignals(true);
      setMinValue(logmin);
      m_minValueBox->blockSignals(false);
    }
    if (maxValue <= 0)
    {
      maxValue = 10.;
      m_maxValueBox->blockSignals(true);
      setMaxValue(maxValue);
      m_maxValueBox->blockSignals(false);
    }
    m_scaleWidget->setScaleDiv(logScaler.transformation(), logScaler.divideScale(logmin, maxValue, 20, 5));
    m_scaleWidget->setColorMap(QwtDoubleInterval(logmin, maxValue), colorMap);
  }
  m_scaleOptions->blockSignals(true);
  m_scaleOptions->setCurrentIndex(m_scaleOptions->findData(type));
  m_scaleOptions->blockSignals(false);
}

/// Send the minValueChanged signal
void ColorMapWidget::minValueChanged()
{
  emit minValueChanged(m_minValueBox->text().toDouble());
}

/// Send the maxValueChanged signal
void ColorMapWidget::maxValueChanged()
{
  emit maxValueChanged(m_maxValueBox->text().toDouble());
}

/**
 * Set a new min value and update the widget.
 * @param value :: The new value
 */
void ColorMapWidget::setMinValue(double value)
{
  setMinValueText(value);
  updateScale();
  if (!m_minValueBox->signalsBlocked())
  {
    minValueChanged();
  }
}

/**
 * Set a new max value and update the widget.
 * @param value :: The new value
 */
void ColorMapWidget::setMaxValue(double value)
{
  setMaxValueText(value);
  updateScale();
  if (!m_maxValueBox->signalsBlocked())
  {
    maxValueChanged();
  }
}

/**
 * Update the min value text box.
 * @param value :: Value to be displayed in the text box.
 */
void ColorMapWidget::setMinValueText(double value)
{
  m_minValueBox->setText(QString::number(value));
}

/**
 * Update the max value text box.
 * @param value :: Value to be displayed in the text box.
 */
void ColorMapWidget::setMaxValueText(double value)
{
  m_maxValueBox->setText(QString::number(value));
}

/**
  * Set the minimum positive value for use with the Log10 scale. Values below this will
  * not be displayed on a Log10 scale.
  */
void ColorMapWidget::setMinPositiveValue(double value)
{
  m_minPositiveValue = value;
}

/**
 * Return the scale type: Log10 or Linear.
 */
int ColorMapWidget::getScaleType()const
{
  return m_scaleOptions->itemData(m_scaleOptions->currentIndex()).toUInt();
}

/**
 * Set the scale type: Log10 or Linear.
 */
void ColorMapWidget::setScaleType(int type)
{
  m_scaleOptions->setCurrentIndex(m_scaleOptions->findData(type));
}

/**
 * Update the colour scale after the range changes.
 */
void ColorMapWidget::updateScale()
{
  double minValue = m_minValueBox->displayText().toDouble();
  double maxValue = m_maxValueBox->displayText().toDouble();
  GraphOptions::ScaleType type = (GraphOptions::ScaleType)m_scaleOptions->itemData(m_scaleOptions->currentIndex()).toUInt();
  if( type == GraphOptions::Linear )
  {
    QwtLinearScaleEngine linScaler;
    m_scaleWidget->setScaleDiv(linScaler.transformation(), linScaler.divideScale(minValue, maxValue,  20, 5));
  }
  else
 {
    QwtLog10ScaleEngine logScaler;    
    double logmin(minValue);
    if( logmin <= 0.0 )
    {
      logmin = m_minPositiveValue;
    }
    m_scaleWidget->setScaleDiv(logScaler.transformation(), logScaler.divideScale(logmin, maxValue, 20, 5));
  }
}

/**
 * Respond to a mouse press event. Start dragging to modify the range (min or max value).
 */
void ColorMapWidget::mousePressEvent(QMouseEvent* e)
{
  QRect rect = m_scaleWidget->rect();
  if (e->x() > rect.left() && e->x() < rect.right())
  {
    m_dragging = true;
    m_y = e->y();
    m_dtype = (m_y > height()/2) ? Bottom : Top;
    QApplication::setOverrideCursor(Qt::ClosedHandCursor);
  }
}

/**
 * Respond to mouse move event. If the left button is down change the min or max.
 */
void ColorMapWidget::mouseMoveEvent(QMouseEvent* e)
{
  if (!m_dragging) return;
 
  double minValue = m_minValueBox->displayText().toDouble();
  double maxValue = m_maxValueBox->displayText().toDouble();

  if (m_dtype == Bottom)
  {
    minValue += double(e->y() - m_y)/height()*(maxValue - minValue);
    setMinValueText(minValue);
  }
  else
  {
    maxValue += double(e->y() - m_y)/height()*(maxValue - minValue);
    setMaxValueText(maxValue);
  }
  m_y = e->y();
  updateScale();
}

/**
 * Respond to a mouse release event. Finish all dragging.
 */
void ColorMapWidget::mouseReleaseEvent(QMouseEvent* /*e*/)
{
  if (!m_dragging) return;
  if (m_dtype == Bottom)
  {
    minValueChanged();
  }
  else
  {
    maxValueChanged();
  }
  QApplication::restoreOverrideCursor();
  m_dragging = false;
}
