#include "MantidVatesSimpleGuiViewWidgets/ColorUpdater.h"
#include "MantidVatesSimpleGuiViewWidgets/ColorSelectionWidget.h"

// Have to deal with ParaView warnings and Intel compiler the hard way.
#if defined(__INTEL_COMPILER)
  #pragma warning disable 1170
#endif

#include <pqChartValue.h>
#include <pqColorMapModel.h>
#include <pqPipelineRepresentation.h>
#include <pqScalarsToColors.h>
#include <pqSMAdaptor.h>
#include <vtkSMProxy.h>

#if defined(__INTEL_COMPILER)
  #pragma warning enable 1170
#endif

#include <QColor>
#include <QList>

#include <limits>
#include <stdexcept>

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{

ColorUpdater::ColorUpdater() :
  autoScaleState(true),
  logScaleState(false),
  minScale(std::numeric_limits<double>::min()),
  maxScale(std::numeric_limits<double>::max())
{
}

ColorUpdater::~ColorUpdater()
{
}

QPair<double, double> ColorUpdater::autoScale(pqPipelineRepresentation *repr)
{
  QPair<double, double> range = repr->getColorFieldRange();
  if (0 == range.first && 1 == range.second)
  {
    throw std::invalid_argument("Bad color scale given");
  }
  pqScalarsToColors *stc = repr->getLookupTable();
  if (NULL != stc)
  {
    stc->setScalarRange(range.first, range.second);
    this->minScale = range.first;
    this->maxScale = range.second;
  }
  else
  {
    throw std::invalid_argument("Cannot get LUT for representation");
  }
  repr->getProxy()->UpdateVTKObjects();
  return range;
}

void ColorUpdater::colorMapChange(pqPipelineRepresentation *repr,
                                  const pqColorMapModel *model)
{
  pqScalarsToColors *lut = repr->getLookupTable();
    if (NULL == lut)
  {
    // Got a bad proxy, so just return
    return;
  }

  // Need the scalar bounds to calculate the color point settings
  QPair<double, double> bounds = lut->getScalarRange();

  vtkSMProxy *lutProxy = lut->getProxy();

  // Set the ColorSpace
  pqSMAdaptor::setElementProperty(lutProxy->GetProperty("ColorSpace"),
                                  model->getColorSpace());
  // Set the NaN color
  QList<QVariant> values;
  QColor nanColor;
  model->getNanColor(nanColor);
  values << nanColor.redF() << nanColor.greenF() << nanColor.blueF();
  pqSMAdaptor::setMultipleElementProperty(lutProxy->GetProperty("NanColor"),
                                          values);

  // Set the RGB points
  QList<QVariant> rgbPoints;
  for(int i = 0; i < model->getNumberOfPoints(); i++)
  {
    QColor rgbPoint;
    pqChartValue fraction;
    model->getPointColor(i, rgbPoint);
    model->getPointValue(i, fraction);
    rgbPoints << fraction.getDoubleValue() * bounds.second << rgbPoint.redF()
              << rgbPoint.greenF() << rgbPoint.blueF();
  }
  pqSMAdaptor::setMultipleElementProperty(lutProxy->GetProperty("RGBPoints"),
                                          rgbPoints);

  lutProxy->UpdateVTKObjects();
}

void ColorUpdater::colorScaleChange(pqPipelineRepresentation *repr,
                                    double min, double max)
{
  if (NULL == repr)
  {
    return;
  }
  pqScalarsToColors *stc = repr->getLookupTable();
  if (NULL != stc)
  {
    stc->setScalarRange(min, max);
    repr->getProxy()->UpdateVTKObjects();
    this->minScale = min;
    this->maxScale = max;
  }
}

void ColorUpdater::logScale(pqPipelineRepresentation *repr, int state)
{
  pqScalarsToColors *lut = repr->getLookupTable();
  if (NULL == lut)
  {
    // Got a bad proxy, so just return
    return;
  }
  QPair<double, double> bounds = lut->getScalarRange();
  // Handle "bug" with lower limit being dropped.
  if (bounds.first != this->minScale)
  {
    lut->setScalarRange(this->minScale, this->maxScale);
  }
  pqSMAdaptor::setElementProperty(lut->getProxy()->GetProperty("UseLogScale"),
                                  state);
  lut->getProxy()->UpdateVTKObjects();
  this->logScaleState = state;
}

/**
 * This function takes information from the color selection widget and
 * sets it into the internal state variables.
 * @param cs : Reference to the color selection widget
 */
void ColorUpdater::updateState(ColorSelectionWidget *cs)
{
  this->autoScaleState = cs->getAutoScaleState();
  this->logScaleState = cs->getLogScaleState();
  this->minScale = cs->getMinRange();
  this->maxScale = cs->getMaxRange();
}

/**
 * @return  the current auto scaling state
 */
bool ColorUpdater::isAutoScale()
{
  return this->autoScaleState;
}

/**
 * @return the current logarithmic scaling state
 */
bool ColorUpdater::isLogScale()
{
  return this->logScaleState;
}

/**
 * @return the current maximum range for the color scaling
 */
double ColorUpdater::getMaximumRange()
{
  return this->maxScale;
}

/**
 * @return the current minimum range for the color scaling
 */
double ColorUpdater::getMinimumRange()
{
  return this->minScale;
}

/**
 * This function prints out the values of the current state of the
 * color updater.
 */
void ColorUpdater::print()
{
  std::cout << "Auto Scale: " << this->autoScaleState << std::endl;
  std::cout << "Log Scale: " << this->logScaleState << std::endl;
  std::cout << "Min Range: " << this->minScale << std::endl;
  std::cout << "Max Range: " << this->maxScale << std::endl;
}

}
}
}
