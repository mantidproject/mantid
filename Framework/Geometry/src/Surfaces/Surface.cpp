#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <complex>
#include <cmath>
#include <list>
#include <vector>
#include <map>
#include <stack>
#include <string>
#include <algorithm>

#include "MantidKernel/Exception.h"
#include "MantidKernel/Strings.h"
#include "MantidGeometry/Math/mathSupport.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/V3D.h"
#include "MantidGeometry/Math/PolyBase.h"
#include "MantidGeometry/Surfaces/BaseVisit.h"
#include "MantidGeometry/Surfaces/Surface.h"

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
GCC_DIAG_OFF(conversion)
// clang-format off
GCC_DIAG_OFF(cast-qual)
// clang-format on
#include <TopoDS_Shape.hxx>
GCC_DIAG_ON(conversion)
// clang-format off
GCC_DIAG_ON(cast-qual)
// clang-format on
#endif

namespace Mantid {

namespace Geometry {

Surface::Surface()
    : Name(-1)
/**
  Constructor
*/
{}

Surface::Surface(const Surface &A)
    : Name(A.Name)
/**
  Copy constructor
  @param A :: Surface to copy
*/
{}

Surface &Surface::operator=(const Surface &A)
/**
  Assignment operator
  @param A :: Surface to copy
  @return *this
*/
{
  if (this != &A) {
    Name = A.Name;
  }
  return *this;
}

Surface::~Surface()
/**
  Destructor
*/
{}

int Surface::side(const Kernel::V3D & /*unused*/) const
/// Surface side : throw AbsObjMethod
{
  throw Kernel::Exception::AbsObjMethod("Surface::side");
}

void Surface::print() const
/**
  Simple print out function for surface header
*/
{
  std::cout << "Surf == " << Name << std::endl;
  return;
}

void Surface::writeHeader(std::ostream &OX) const
/**
  Writes out the start of an MCNPX surface description .
  Does not check the length etc
  @param OX :: Output stream
*/
{
  OX << Name << " ";
  return;
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
