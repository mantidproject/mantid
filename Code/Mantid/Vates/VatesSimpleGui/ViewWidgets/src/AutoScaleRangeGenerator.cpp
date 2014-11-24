#include "MantidVatesSimpleGuiViewWidgets/AutoScaleRangeGenerator.h"

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
#include <vtkSMPropertyHelper.h>

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{
  /**
   * Note that the mode is currently set to standard.
   */
  AutoScaleRangeGenerator::AutoScaleRangeGenerator() : mode(STANDARD), defaultValue(1e-2) {};

  /**
   * Get the auto color scale which depends on the mode setting.
   * @retruns A VsiColorScale data structure which contains information 
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
        colorScaleContainer.useLogScale= FALSE;

        break;
    }

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

    bool initialSetting = TRUE;

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

          initialSetting = FALSE;
        }
      }
    }

    // Set the color scale output
    VsiColorScale vsiColorScale;

    vsiColorScale.minValue = minValue;
    vsiColorScale.maxValue = minValue + 0.1*(maxValue - minValue);
    vsiColorScale.useLogScale = TRUE;

    return vsiColorScale;
  }

  /**
   * Extract the min and max values of a source. If we are dealing with a filter,
   * then look further upstream for the information. At the end of the pipeline 
   * we have to encounter a source with the desired properties. 
   * Note that this assumes a unique source.
   * @param source A pointer to a source.
   * @param minValueBuffer A reference to a min value buffer.
   * @param maxValueBuffer A reference to a max value buffer.
   */
  void AutoScaleRangeGenerator::setMinBufferAndMaxBuffer(pqPipelineSource* source, double& minValue, double& maxValue)
  {
    pqPipelineSource* pqSource = source;

    bool isFilter = true;

    while(isFilter)
    {
      // If we are dealing with a filter, we can cast it.
      pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(pqSource);

      if (!filter)
      {
        isFilter = false;

        minValue = vtkSMPropertyHelper(pqSource->getProxy(),
                                       "MinValue").GetAsDouble();

        maxValue = vtkSMPropertyHelper(pqSource->getProxy(),
                                       "MaxValue").GetAsDouble();
      } 
      else
      {
        // We expect one input, if not provide defailt values
        if (filter->getInputCount() != 1)
        {
          minValue = this->defaultValue;
          maxValue = this->defaultValue;
          isFilter = false;
        } 
        else
        {
          QList<pqOutputPort*> outputPorts = filter->getAllInputs();

          pqSource = outputPorts[0]->getSource();
        }
      }
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

    sources = smModel->findItems<pqPipelineSource *>(server);

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
}
}
}