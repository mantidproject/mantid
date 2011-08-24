#include "MantidVatesSimpleGuiViewWidgets/ColorUpdater.h"

#include <pqChartValue.h>
#include <pqColorMapModel.h>
#include <pqPipelineRepresentation.h>
#include <pqScalarsToColors.h>
#include <pqSMAdaptor.h>
#include <vtkSMProxy.h>

#include <QColor>
#include <QList>

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{

ColorUpdater::ColorUpdater()
{
}

ColorUpdater::~ColorUpdater()
{
}

QPair<double, double> ColorUpdater::autoScale(pqPipelineRepresentation *repr)
{
  QPair<double, double> range = repr->getColorFieldRange();
  repr->getLookupTable()->setScalarRange(range.first, range.second);
  repr->getProxy()->UpdateVTKObjects();
  return range;
}

void ColorUpdater::colorMapChange(pqPipelineRepresentation *repr,
                                  const pqColorMapModel *model)
{
  pqScalarsToColors *lut = repr->getLookupTable();
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
  repr->getLookupTable()->setScalarRange(min, max);
  repr->getProxy()->UpdateVTKObjects();
}

void ColorUpdater::logScale(pqPipelineRepresentation *repr, int state)
{
  pqScalarsToColors *lut = repr->getLookupTable();
  pqSMAdaptor::setElementProperty(lut->getProxy()->GetProperty("UseLogScale"),
                                  state);
  lut->getProxy()->UpdateVTKObjects();
}

}
}
}
