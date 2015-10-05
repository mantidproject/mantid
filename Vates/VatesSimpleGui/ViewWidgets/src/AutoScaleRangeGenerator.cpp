#include "MantidVatesSimpleGuiViewWidgets/AutoScaleRangeGenerator.h"
#include "MantidQtAPI/MdConstants.h"
// Have to deal with ParaView warnings and Intel compiler the hard way.
#if defined(__INTEL_COMPILER)
  #pragma warning disable 1170
#endif

#include <pqServer.h>
#include <pqActiveObjects.h>
#include <pqServerManagerModel.h>
#include <pqApplicationCore.h>
#include <pqOutputPort.h>
#include <pqPipelineFilter.h>
#include <pqPipelineSource.h>
#include <pqPipelineRepresentation.h>
#include <pqScalarsToColors.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>
#include <vtkSMDoubleVectorProperty.h>

#if defined(__INTEL_COMPILER)
  #pragma warning enable 1170
#endif

#include <cfloat>
#include <QPair>
#include "MantidQtAPI/MdSettings.h"

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{
  /**
   * Note that the mode is currently set to standard.
   */
  AutoScaleRangeGenerator::AutoScaleRangeGenerator() : mode(STANDARD), defaultValue(1e-2) 
  {
    //Set the initial log scale state due to the mode
    m_mdSettings.setLastSessionLogScale(getLogScale());
  }

  /**
   * Gets the log scale for the mode
   * @returns The log scale state.
   */
  bool AutoScaleRangeGenerator::getLogScale()
  {   
    bool logScale = false;

    switch(mode)
    {
      case(STANDARD):
        logScale = false;
        break;

      case(TECHNIQUEDEPENDENT):
        // Implement technique-dependence here

      case(OFFSET):
          // Implement color scale which accounts for noise floor here.

      default:
        logScale= false;
        break;
    }

    return logScale;
  }

  /**
   * Get the auto color scale which depends on the mode setting.
   * @returns A VsiColorScale data structure which contains information 
   *          regarding the min and max value as well as if the log 
   *          scale is being used.
   */
  VsiColorScale AutoScaleRangeGenerator::getColorScale()
  {
    VsiColorScale colorScaleContainer;

    // Select a colorscale depending on the selected mode
    switch(mode)
    {
      case(STANDARD):
        colorScaleContainer = getStandardColorScale();
        break;

      case(TECHNIQUEDEPENDENT):
        // Implement technique-dependence here
      case(OFFSET):
          // Implement color scale which accounts for noise floor here.
      default:
        colorScaleContainer.maxValue = 1.0;
        colorScaleContainer.minValue = 0.0;
        colorScaleContainer.useLogScale= false;

        break;
    }

    // Set the colorScale Container
    colorScaleContainer.useLogScale = m_mdSettings.getLastSessionLogScale();


    // Make sure that the color scale is valid, and if not set default values.
    sanityCheck(colorScaleContainer);

    return colorScaleContainer;
  }

  /**
   * The standard way of creating a colorscale entity. The minimum 
   * and maxiumum data value of all sources is obtained. The minimum 
   * color scale value is set to the minimum data value. The maximum 
   * color scale value is set to 10% of the maximum data value. The
   * scale is set to be logarithmic.
   *
   * @returns A color scale entity.
   */
  VsiColorScale AutoScaleRangeGenerator::getStandardColorScale()
  {
    // Select any number larger than 1 to start with
    double maxValue= this->defaultValue;
    double maxValueBuffer = this->defaultValue;
    
    double minValue = this->defaultValue; 
    double minValueBuffer = this->defaultValue;

    bool initialSetting = true;

    QList<pqPipelineSource *> sources = getAllPVSources();

    pqView* activeView = pqActiveObjects::instance().activeView();

    // Check all sources for the maximum and minimum value
    for (QList<pqPipelineSource *>::iterator source = sources.begin();
         source != sources.end(); ++source)
    {
      // Check if the pipeline representation of the source for the active view is visible
      pqDataRepresentation* representation = (*source)->getRepresentation(activeView);

      if (representation)
      {
        bool isVisible = representation->isVisible();

        if (isVisible)
        {
          setMinBufferAndMaxBuffer((*source), minValueBuffer, maxValueBuffer);

          if (initialSetting || maxValueBuffer > maxValue)
          {
            maxValue = maxValueBuffer;
          }

          if (initialSetting || minValueBuffer < minValue)
          {
            minValue = minValueBuffer;
          }

          initialSetting = false;
        }
      }
    }

    // Set the color scale output
    VsiColorScale vsiColorScale;

    // Initialize log scale to false
    vsiColorScale.useLogScale = false;

    // If either the min or max value are at the end of the double spectrum, we might only have a peak Ws visible, 
    // we need to hedge for that
    if (minValue == DBL_MAX || maxValue == -DBL_MAX) {
      minValue = defaultValue;
      maxValue = defaultValue;
    }

    // Account for possible negative data. If min value is negative and max value is larger than 100, then set to default
    // else set to three orders of magnitude smaller than the max value
    if (minValue < 0 && maxValue > 100)
    {
      minValue = this->defaultValue;
    } else if (minValue < 0 && maxValue < 100)
    {
      minValue = maxValue*0.001;
    } 

    vsiColorScale.minValue = minValue;
    vsiColorScale.maxValue = minValue + (maxValue - minValue)*m_mdConstants.getColorScaleStandardMax();

    return vsiColorScale;
  }

  /**
   * Extract the min and max values of a source. If we are dealing with a filter,
   * then look further upstream for the information. At the end of the pipeline 
   * we have to encounter a source with the desired properties. 
   * Note that this assumes a unique source.
   * @param source A pointer to a source.
   * @param minValue A reference to a min value buffer.
   * @param maxValue A reference to a max value buffer.
   */
  void AutoScaleRangeGenerator::setMinBufferAndMaxBuffer(pqPipelineSource* source, double& minValue, double& maxValue)
  {
    // Make sure that the pipeline properties are up to date
    vtkSMProxy* proxy = source->getProxy();
    proxy->UpdateVTKObjects();
    proxy->UpdatePropertyInformation();
    source->updatePipeline();

    // Check if source is custom filter
    if (QString(proxy->GetXMLName()).contains("MantidParaViewScaleWorkspace") ||
        QString(proxy->GetXMLName()).contains("MDEWRebinningCutter") ||
        QString(proxy->GetXMLName()).contains("MantidParaViewSplatterPlot") ||
        QString(proxy->GetXMLName()).contains("MantidParaViewPeaksFilter"))

    {
      minValue = vtkSMPropertyHelper(proxy,"MinValue").GetAsDouble();
      maxValue = vtkSMPropertyHelper(proxy,"MaxValue").GetAsDouble();

      return;
    }

    // Check if source is custom source (MDHisto or MDEvent)
    if (QString(proxy->GetXMLName()).contains("MDEW Source") ||
        QString(proxy->GetXMLName()).contains("MDHW Source"))
    {
      minValue = vtkSMPropertyHelper(proxy,"MinValue").GetAsDouble();
      maxValue = vtkSMPropertyHelper(proxy,"MaxValue").GetAsDouble();

      return;
    }

    // Check if Peak Workspace. This workspace should not contribute to colorscale
    if (QString(proxy->GetXMLName()).contains("Peaks Source") ||
        QString(proxy->GetXMLName()).contains("SinglePeakMarkerSource") ||
        QString(proxy->GetXMLName()).contains("Threshold") ||
        QString(proxy->GetXMLName()).contains("ProbePoint"))
    {
      minValue = DBL_MAX;
      maxValue = -DBL_MAX;

      return;
    }

    // Otherwise get the data range of the representation for the active view
    pqPipelineRepresentation* pipelineRepresentation = qobject_cast<pqPipelineRepresentation*>(source->getRepresentation(pqActiveObjects::instance().activeView()));

    if (pipelineRepresentation)
    {
      QPair<double, double> range = pipelineRepresentation->getLookupTable()->getScalarRange();

      minValue = range.first;
      maxValue = range.second;
    }
  }

  /**
   * Get all sources from the PV server
   */
  QList<pqPipelineSource *> AutoScaleRangeGenerator::getAllPVSources()
  {
    pqServer *server = pqActiveObjects::instance().activeServer();

    pqServerManagerModel *smModel = pqApplicationCore::instance()->getServerManagerModel();

    QList<pqPipelineSource *> sources;

    if (server)
    {
      sources = smModel->findItems<pqPipelineSource *>(server);
    }

    return sources;
  }

  /**
   * Sanity check for the color scale, e.g. no 0 for logarithmic scaling. 
   * @param colorscale A colorscale container
   * @returns A colorscale container
   */
  void AutoScaleRangeGenerator::sanityCheck(VsiColorScale& colorscale)
  {
    // Make sure that the min value is larger than 0 for log scales
    if (colorscale.useLogScale && colorscale.minValue <= 0.0)
    {
      colorscale.minValue = this->defaultValue;
    }

    // Make sure that the min value is larger than 0 for log scales
    if (colorscale.useLogScale && colorscale.maxValue <= 0.0)
    {
      colorscale.maxValue = this->defaultValue;
    }
  }

  /**
   * Initializes the color scale state, in particular if it is a log scale.
   */
  void AutoScaleRangeGenerator::initializeColorScale()
  {
    m_mdSettings.setLastSessionLogScale(getLogScale());
  }

  /**
   * Update the log scale setting
   * @param logScale The log scale setting
   */
  void AutoScaleRangeGenerator::updateLogScaleSetting(bool logScale)
  {
    m_mdSettings.setLastSessionLogScale(logScale);
  }
}
}
}
