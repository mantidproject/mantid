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
#include "MantidKernel/Logger.h"

#include "MantidKernel/Support.h"
#include "MantidGeometry/Math/mathSupport.h"
#include "MantidGeometry/Math/Matrix.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Math/PolyBase.h"
#include "MantidGeometry/Surfaces/BaseVisit.h"
#include "MantidGeometry/Surfaces/Surface.h"

namespace Mantid
{

namespace Geometry
{

Kernel::Logger& Surface::PLog = Kernel::Logger::get("Surface"); 

Surface::Surface() : 
  Name(-1)
  /**
    Constructor
  */
{}

Surface::Surface(const Surface& A) : 
  Name(A.Name)
  /**
    Copy constructor
    @param A :: Surface to copy
  */
{ }


Surface&
Surface::operator=(const Surface& A)
  /**
    Assignment operator
    @param A :: Surface to copy
    @return *this
  */
{
  if (this!=&A)
    {
      Name=A.Name;
    }
  return *this;
}

Surface::~Surface()
  /**
    Destructor
  */
{}

int
Surface::side(const Geometry::V3D&) const
  /// Surface side : throw AbsObjMethod
{
  throw Kernel::Exception::AbsObjMethod("Surface::side");
}

void 
Surface::print() const
  /**
    Simple print out function for surface header 
  */
{
  std::cout<<"Surf == "<<Name<<std::endl;
  return;
}

void
Surface::writeHeader(std::ostream& OX) const
  /**
    Writes out the start of an MCNPX surface description .
    Does not check the length etc
    @param OX :: Output stream
  */
{
  OX<<Name<<" ";
  return;
}
  

void
Surface::write(std::ostream& out) const  
/**
    The writes the data to the output stream.
    @param out :: The output stream 
  */
{
  (void) out; //Avoid compiler warning
  throw Kernel::Exception::AbsObjMethod("Surface::write");
}
  
  
}  // NAMESPACE Geometry

} // NAMESPACE Mantid




