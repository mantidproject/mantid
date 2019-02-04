// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/ConvertToDiffractionMDWorkspace2.h"

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
DECLARE_ALGORITHM(ConvertToDiffractionMDWorkspace2)

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ConvertToDiffractionMDWorkspace2::init() {
  // initilise common properties between versions
  BaseConvertToDiffractionMDWorkspace::init();

  std::vector<double> extents = {-50.0, 50.0};
  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>("Extents", std::move(extents)),
      "A comma separated list of min, max for each dimension,\n"
      "specifying the extents of each dimension. Optional, default "
      "+- 50 in each dimension.");
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
void ConvertToDiffractionMDWorkspace2::convertExtents(
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
  } else
    throw std::invalid_argument(
        "You must specify either 2 or 6 extents (min,max).");
}

} // namespace MDAlgorithms
} // namespace Mantid
