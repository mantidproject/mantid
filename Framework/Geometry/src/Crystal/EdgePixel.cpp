// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Crystal/EdgePixel.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"

namespace Mantid::Geometry {

/**
  @param  inst         Instrument
  @param  bankName     Name of detector bank
  @param  col          Column number containing peak
  @param  row          Row number containing peak
  @param  Edge         Number of edge points for each bank
  @return True if peak is on edge
*/
bool edgePixel(const Mantid::Geometry::Instrument_const_sptr &inst, const std::string &bankName, int col, int row,
               int Edge) {
  if (bankName == "None")
    return false;
  std::shared_ptr<const Geometry::IComponent> parent = inst->getComponentByName(bankName);
  if (parent->type() == "RectangularDetector") {
    std::shared_ptr<const Geometry::RectangularDetector> RDet =
        std::dynamic_pointer_cast<const Geometry::RectangularDetector>(parent);

    return col < Edge || col >= (RDet->xpixels() - Edge) || row < Edge || row >= (RDet->ypixels() - Edge);
  } else {
    // get the component info
    auto const &pmap = inst->getParameterMap();
    auto const &wrappers = inst->makeBeamline(*pmap);
    auto const &compInfo = wrappers.first;

    // get the children and grandchildren from the component info
    size_t parentIndex = compInfo->indexOfAny(bankName);
    auto children = compInfo->children(parentIndex);
    int startI = 1;
    if (!children.empty() && compInfo->name(children[0]) == "sixteenpack") {
      startI = 0;
      children = compInfo->children(children[0]);
    }
    auto grandchildren = compInfo->children(children[0]);

    // calculate if the pixel is on the edge of the bank
    auto NROWS = static_cast<int>(grandchildren.size());
    auto NCOLS = static_cast<int>(children.size());
    // Wish pixels and tubes start at 1 not 0
    return col - startI < Edge || col - startI >= (NCOLS - Edge) || row - startI < Edge ||
           row - startI >= (NROWS - Edge);
  }
  return false;
}
} // namespace Mantid::Geometry
