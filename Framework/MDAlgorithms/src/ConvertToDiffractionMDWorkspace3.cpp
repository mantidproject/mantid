// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/ConvertToDiffractionMDWorkspace3.h"

#include "MantidAPI/IMDEventWorkspace.h"

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"

#include "MantidMDAlgorithms/ConvertToMDMinMaxLocal.h"
#include "MantidMDAlgorithms/MDTransfFactory.h"
#include "MantidMDAlgorithms/MDWSTransform.h"

#include <algorithm>
#include <limits>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertToDiffractionMDWorkspace3)

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ConvertToDiffractionMDWorkspace3::init() {
  // initilise common properties between versions
  BaseConvertToDiffractionMDWorkspace::init();

  std::vector<double> extents;
  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>("Extents", std::move(extents)),
      "A comma separated list of min, max for each dimension,\n"
      "specifying the extents of each dimension. Optional, default "
      "will use ConvertToMDMinMaxLocal to calculate extents for each "
      "dimension.");
  setPropertyGroup("Extents", getBoxSettingsGroupName());
}

/** Splits extents accepted by convertToDiffreactionMD workspace in the form
 *min1,max1 or min1,max1,min2,max2,min3,max3
 *   into tso vectors min(3),max(3) accepted by convertToMD
 *  @param  Extents  -- the vector of extents consititing of 2 or 6 elements
 *  @param minVal   -- 3-vector of minimal values for 3 processed dimensions
 *  @param maxVal   -- 3-vector of maximal values for 3 processed dimensions
 *
 * @return minVal and maxVal -- two vectors with minimal and maximal values of
 *the momentums in the target workspace.
 */
void ConvertToDiffractionMDWorkspace3::convertExtents(
    const std::vector<double> &Extents, std::vector<double> &minVal,
    std::vector<double> &maxVal) {
  minVal.resize(3);
  maxVal.resize(3);
  if (Extents.size() == 2) {
    for (size_t d = 0; d < 3; d++) {
      minVal[d] = Extents[0];
      maxVal[d] = Extents[1];
    }
  } else if (Extents.size() == 6) {
    for (size_t d = 0; d < 3; d++) {
      minVal[d] = Extents[2 * d + 0];
      maxVal[d] = Extents[2 * d + 1];
    }
  } else if (Extents.empty()) {
    calculateExtentsFromData(minVal, maxVal);
  } else
    throw std::invalid_argument(
        "You must specify either 2 or 6 extents (min,max).");
}

/** Calculate the extents to use for the conversion from the input workspace.
 *
 * @param minVal :: the minimum bounds of the MD space.
 * @param maxVal :: the maximum bounds of the MD space.
 */
void ConvertToDiffractionMDWorkspace3::calculateExtentsFromData(
    std::vector<double> &minVal, std::vector<double> &maxVal) {
  auto alg = createChildAlgorithm("ConvertToMDMinMaxLocal");
  alg->initialize();
  alg->setProperty<MatrixWorkspace_sptr>("InputWorkspace",
                                         getProperty("InputWorkspace"));
  alg->setPropertyValue("QDimensions", "Q3D");
  std::vector<std::string> dE_modes = Kernel::DeltaEMode::availableTypes();
  alg->setPropertyValue("dEAnalysisMode",
                        dE_modes[Kernel::DeltaEMode::Elastic]);

  std::string TargetFrame, Scaling;
  convertFramePropertyNames(getPropertyValue("OutputDimensions"), TargetFrame,
                            Scaling);
  alg->setProperty("Q3DFrames", TargetFrame);
  alg->setProperty("QConversionScales", Scaling);

  alg->execute();

  minVal = alg->getProperty("MinValues");
  maxVal = alg->getProperty("MaxValues");

  // If the calculation produced +/- infinity as one of the extents
  // replace this with a more reasonable value.
  const auto INF = std::numeric_limits<double>::infinity();
  const auto MAX_DBL = std::numeric_limits<double>::max();
  const auto DEFAULT_BOUND = 50.0;

  std::replace(minVal.begin(), minVal.end(), -INF, -DEFAULT_BOUND);
  std::replace(maxVal.begin(), maxVal.end(), INF, DEFAULT_BOUND);
  std::replace(minVal.begin(), minVal.end(), MAX_DBL, -DEFAULT_BOUND);
  std::replace(maxVal.begin(), maxVal.end(), -MAX_DBL, DEFAULT_BOUND);

  // Increases bounds by a small margin and ensures they aren't too close to
  // zero. This is to prevent events from being incorrectly discarded as out
  // of bounds by calcMatrixCoord functions (or, if fixed there, later causing
  // further issues in MDGridBox::calculateChildIndex due to events sitting
  // on maximum boundaries)
  for (size_t i = 0; i < maxVal.size(); ++i) {
    minVal[i] *= (1 - 1.e-5 * boost::math::sign(minVal[i]));
    if (fabs(minVal[i]) < 1.e-5)
      minVal[i] = -1.e-5;
    maxVal[i] *= (1 + 1.e-5 * boost::math::sign(maxVal[i]));
    if (fabs(maxVal[i]) < 1.e-5)
      maxVal[i] = 1.e-5;
  }
}

} // namespace MDAlgorithms
} // namespace Mantid
