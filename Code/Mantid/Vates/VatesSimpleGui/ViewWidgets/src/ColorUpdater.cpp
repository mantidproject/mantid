#include "MantidVatesSimpleGuiViewWidgets/ColorUpdater.h"
#include "MantidVatesSimpleGuiViewWidgets/ColorSelectionWidget.h"
#include "MantidVatesSimpleGuiViewWidgets/AutoScaleRangeGenerator.h"

// Have to deal with ParaView warnings and Intel compiler the hard way.
#if defined(__INTEL_COMPILER)
  #pragma warning disable 1170
#endif

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqChartValue.h>
#include <pqColorMapModel.h>
#include <pqDataRepresentation.h>
#include <pqPipelineRepresentation.h>
#include <pqScalarsToColors.h>
#include <pqServerManagerModel.h>
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

/**
 * Set the lookup table to the autoscale values.
 * @returns A struct which contains the parameters of the looup table.
 */
VsiColorScale ColorUpdater::autoScale()
{
  // Get the custom auto scale.
  VsiColorScale vsiColorScale =  this->autoScaleRangeGenerator.getColorScale();

  // Set the color scale for all sources
  this->minScale = vsiColorScale.minValue;
  this->maxScale = vsiColorScale.maxValue;

  // If the view 

  this->logScaleState = vsiColorScale.useLogScale;

  // Update the lookup tables, i.e. react to a color scale change
  colorScaleChange(this->minScale, this->maxScale);

  return vsiColorScale;
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

/**
 * React to a change of the color scale settings.
 * @param min The lower end of the color scale.
 * @param max The upper end of the color scale.
 */
void ColorUpdater::colorScaleChange(double min, double max)
{
  this->minScale = min;
  this->maxScale = max;

  try
  {
    // Update for all sources and all reps
    pqServer *server = pqActiveObjects::instance().activeServer();
    pqServerManagerModel *smModel = pqApplicationCore::instance()->getServerManagerModel();
    QList<pqPipelineSource *> sources = smModel->findItems<pqPipelineSource *>(server);
    QList<pqPipelineSource *>::Iterator source;

    // For all sources
    for(QList<pqPipelineSource*>::iterator source = sources.begin(); source != sources.end(); ++source)
    {
      QList<pqView*> views = (*source)->getViews();

      // For all views
      for (QList<pqView*>::iterator view = views.begin(); view != views.end(); ++view)
      {
        QList<pqDataRepresentation*> reps =  (*source)->getRepresentations((*view));

        // For all representations
        for (QList<pqDataRepresentation*>::iterator rep = reps.begin(); rep != reps.end(); ++rep)
        {
          this->updateLookupTable(*rep);
        }
      }
    }
  }
  catch(std::invalid_argument &)
  {

    return;
  }
}

/**
 * Update the lookup table.
 * @param representation The representation for which the lookup table is updated.
 */
void ColorUpdater::updateLookupTable(pqDataRepresentation* representation)
{
  pqScalarsToColors* lookupTable = representation->getLookupTable();

  if (NULL != lookupTable)
  {
    // Set the scalar range values
    lookupTable->setScalarRange(this->minScale, this->maxScale);

    // Set the logarithmic scale
    pqSMAdaptor::setElementProperty(lookupTable->getProxy()->GetProperty("UseLogScale"),
                                     this->logScaleState);

    // Need to set a lookup table lock here. This does not affect setScalarRange, 
    // but blocks setWholeScalarRange which gets called by ParaView overrides our
    // setting when a workspace is loaded for the first time.
    lookupTable->setScalarRangeLock(TRUE);

    representation->getProxy()->UpdateVTKObjects();
    representation->renderViewEventually();
  } else
  {
    throw std::invalid_argument("Cannot get LUT for representation");
  }
}

/**
 * React to changing the log scale option
 * @param The state to which the log scale is being changed.
 */
void ColorUpdater::logScale(int state)
{
  this->logScaleState = state;

  this->colorScaleChange(this->minScale, this->maxScale);
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
  this->autoScaleRangeGenerator.updateLogScaleSetting(this->logScaleState);
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

/**
 * Initializes the color scale
 */
void ColorUpdater::initializeColorScale()
{
  autoScaleRangeGenerator.initializeColorScale();
}
}
}
}
