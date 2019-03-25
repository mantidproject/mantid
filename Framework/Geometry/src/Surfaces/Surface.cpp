// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include <algorithm>
#include <complex>
#include <fstream>
#include <iomanip>
#include <list>
#include <map>
#include <sstream>
#include <stack>
#include <vector>

#include "MantidGeometry/Math/mathSupport.h"
#include "MantidGeometry/Surfaces/BaseVisit.h"
#include "MantidGeometry/Surfaces/Surface.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/V3D.h"

#ifdef ENABLE_OPENCASCADE
// Opencascade defines _USE_MATH_DEFINES without checking whether it is already
// used.
// Undefine it here before we include the headers to avoid a warning
#ifdef _MSC_VER
#undef _USE_MATH_DEFINES
#ifdef M_SQRT1_2
#undef M_SQRT1_2
#endif
#endif

#include "MantidKernel/WarningSuppressions.h"
GNU_DIAG_OFF("conversion")
GNU_DIAG_OFF("cast-qual")
#include <TopoDS_Shape.hxx>
GNU_DIAG_ON("conversion")
GNU_DIAG_ON("cast-qual")
#endif

namespace Mantid {

namespace Geometry {
namespace {
Kernel::Logger logger("Surface");
}

Surface::Surface()
    : Name(-1)
/**
  Constructor
*/
{}

int Surface::side(const Kernel::V3D &) const
/// Surface side : throw AbsObjMethod
{
  throw Kernel::Exception::AbsObjMethod("Surface::side");
}

void Surface::print() const
/**
  Simple print out function for surface header
*/
{
  logger.debug() << "Surf == " << Name << '\n';
}

void Surface::writeHeader(std::ostream &OX) const
/**
  Writes out the start of an MCNPX surface description .
  Does not check the length etc
  @param OX :: Output stream
*/
{
  OX << Name << " ";
}

void Surface::write(std::ostream &out) const
/**
    The writes the data to the output stream.
    @param out :: The output stream
  */
{
  (void)out; // Avoid compiler warning
  throw Kernel::Exception::AbsObjMethod("Surface::write");
}

#ifdef ENABLE_OPENCASCADE
TopoDS_Shape Surface::createShape() { return TopoDS_Shape(); }
#endif
} // NAMESPACE Geometry

} // NAMESPACE Mantid
