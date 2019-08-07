// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Crystal/EdgePixel.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"

namespace Mantid {
namespace Geometry {

/**
  @param  inst         Instrument
  @param  bankName     Name of detector bank
  @param  col          Column number containing peak
  @param  row          Row number containing peak
  @param  Edge         Number of edge points for each bank
  @return True if peak is on edge
*/
bool edgePixel(Mantid::Geometry::Instrument_const_sptr inst,
               std::string bankName, int col, int row, int Edge) {
  if (bankName == "None")
    return false;
  boost::shared_ptr<const Geometry::IComponent> parent =
      inst->getComponentByName(bankName);
  if (parent->type() == "RectangularDetector") {
    boost::shared_ptr<const Geometry::RectangularDetector> RDet =
        boost::dynamic_pointer_cast<const Geometry::RectangularDetector>(
            parent);

    return col < Edge || col >= (RDet->xpixels() - Edge) || row < Edge ||
           row >= (RDet->ypixels() - Edge);
  } else {
    std::vector<Geometry::IComponent_const_sptr> children;
    boost::shared_ptr<const Geometry::ICompAssembly> asmb =
        boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(parent);
    asmb->getChildren(children, false);
    int startI = 1;
    if (children[0]->getName() == "sixteenpack") {
      startI = 0;
      parent = children[0];
      children.clear();
      asmb = boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(parent);
      asmb->getChildren(children, false);
    }
    boost::shared_ptr<const Geometry::ICompAssembly> asmb2 =
        boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(children[0]);
    std::vector<Geometry::IComponent_const_sptr> grandchildren;
    asmb2->getChildren(grandchildren, false);
    auto NROWS = static_cast<int>(grandchildren.size());
    auto NCOLS = static_cast<int>(children.size());
    // Wish pixels and tubes start at 1 not 0
    return col - startI < Edge || col - startI >= (NCOLS - Edge) ||
           row - startI < Edge || row - startI >= (NROWS - Edge);
  }
  return false;
}
} // namespace Geometry
} // namespace Mantid
