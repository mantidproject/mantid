// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidVatesSimpleGuiViewWidgets/AutoScaleRangeGenerator.h"
#include "MantidQtWidgets/Common/MdConstants.h"
// Have to deal with ParaView warnings and Intel compiler the hard way.
#if defined(__INTEL_COMPILER)
#pragma warning disable 1170
#endif

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqOutputPort.h"
#include "pqPipelineFilter.h"
#include "pqPipelineRepresentation.h"
#include "pqPipelineSource.h"
#include "pqScalarsToColors.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"

#if defined(__INTEL_COMPILER)
#pragma warning enable 1170
#endif

#include "MantidQtWidgets/Common/MdSettings.h"
#include <QPair>
#include <cfloat>

namespace Mantid {
namespace Vates {
namespace SimpleGui {
/**
 * Note that the mode is currently set to standard.
 */
AutoScaleRangeGenerator::AutoScaleRangeGenerator()
    : mode(STANDARD), defaultValue(1e-2) {
  // Set the initial log scale state due to the mode
  m_mdSettings.setLastSessionLogScale(getLogScale());
}

/**
 * Gets the log scale for the mode
 * @returns The log scale state.
 */
bool AutoScaleRangeGenerator::getLogScale() {
  bool logScale = false;

  switch (mode) {
  case (STANDARD):
    logScale = false;
    break;

  case (TECHNIQUEDEPENDENT):
    // Implement technique-dependence here

  case (OFFSET):
    // Implement color scale which accounts for noise floor here.

  default:
    logScale = false;
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
VsiColorScale AutoScaleRangeGenerator::getColorScale() {
  VsiColorScale colorScaleContainer;

  // Select a colorscale depending on the selected mode
  switch (mode) {
  case (STANDARD):
    colorScaleContainer = getStandardColorScale();
    break;

  case (TECHNIQUEDEPENDENT):
  // Implement technique-dependence here
  case (OFFSET):
  // Implement color scale which accounts for noise floor here.
  default:
    colorScaleContainer.maxValue = 1.0;
    colorScaleContainer.minValue = 0.0;

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
VsiColorScale AutoScaleRangeGenerator::getStandardColorScale() {
  // Select any number larger than 1 to start with
  double maxValue = -DBL_MAX;
  double minValue = DBL_MAX;

  pqView *activeView = pqActiveObjects::instance().activeView();

  // Check all sources for the maximum and minimum value
  const QList<pqPipelineSource *> sources = getAllPVSources();
  for (pqPipelineSource *source : sources) {
    // Check if the pipeline representation of the source for the active view is
    // visible
    pqDataRepresentation *drep = source->getRepresentation(activeView);
    if (drep && drep->isVisible()) {
      auto info = vtkSMPVRepresentationProxy::GetArrayInformationForColorArray(
          drep->getProxy());
      if (info) {
        double range[2];
        info->GetComponentFiniteRange(-1, range);
        minValue = std::min(minValue, range[0]);
        maxValue = std::max(maxValue, range[1]);
      }
    }
  }

  // Set the color scale output
  VsiColorScale vsiColorScale;

  // Initialize log scale to false
  vsiColorScale.useLogScale = false;

  // If either the min or max value are at the end of the double spectrum, we
  // might only have a peak Ws visible,
  // we need to hedge for that
  if (minValue == DBL_MAX || maxValue == -DBL_MAX) {
    minValue = defaultValue;
    maxValue = defaultValue;
  }

  // Account for possible negative data. If min value is negative and max value
  // is larger than 100, then set to default
  // else set to three orders of magnitude smaller than the max value
  if (minValue < 0 && maxValue > 100) {
    minValue = this->defaultValue;
  } else if (minValue < 0 && maxValue < 100) {
    minValue = maxValue * 0.001;
  }

  vsiColorScale.minValue = minValue;
  vsiColorScale.maxValue =
      minValue +
      (maxValue - minValue) * m_mdConstants.getColorScaleStandardMax();

  return vsiColorScale;
}

/**
 * Get all sources from the PV server
 */
QList<pqPipelineSource *> AutoScaleRangeGenerator::getAllPVSources() {
  pqServer *server = pqActiveObjects::instance().activeServer();

  pqServerManagerModel *smModel =
      pqApplicationCore::instance()->getServerManagerModel();

  QList<pqPipelineSource *> sources;

  if (server) {
    sources = smModel->findItems<pqPipelineSource *>(server);
  }

  return sources;
}

/**
 * Sanity check for the color scale, e.g. no 0 for logarithmic scaling.
 * @param colorscale A colorscale container
 * @returns A colorscale container
 */
void AutoScaleRangeGenerator::sanityCheck(VsiColorScale &colorscale) {
  // Make sure that the min value is larger than 0 for log scales
  if (colorscale.useLogScale && colorscale.minValue <= 0.0) {
    colorscale.minValue = this->defaultValue;
  }

  // Make sure that the min value is larger than 0 for log scales
  if (colorscale.useLogScale && colorscale.maxValue <= 0.0) {
    colorscale.maxValue = this->defaultValue;
  }
}

/**
 * Initializes the color scale state, in particular if it is a log scale.
 */
void AutoScaleRangeGenerator::initializeColorScale() {
  m_mdSettings.setLastSessionLogScale(getLogScale());
}

/**
 * Update the log scale setting
 * @param logScale The log scale setting
 */
void AutoScaleRangeGenerator::updateLogScaleSetting(bool logScale) {
  m_mdSettings.setLastSessionLogScale(logScale);
}
} // namespace SimpleGui
} // namespace Vates
} // namespace Mantid
