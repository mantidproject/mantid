#include <iostream> 
#include "MantidDataObjects/Histogram1D.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/WorkspaceFactory.h"

namespace Mantid
{
namespace DataObjects
{

/**
 Constructor
 */
Histogram1D::Histogram1D()
{}

/**
 Copy constructor
 @param A :: Histogram to copy
 */
Histogram1D::Histogram1D(const Histogram1D& A) :
  refX(A.refX), refY(A.refY), refE(A.refE), refDx(A.refDx)
{}

/**
 Assignment operator
 @param A :: Histogram to copy
 @return *this
 */
Histogram1D& Histogram1D::operator=(const Histogram1D& A)
{
  if (this!=&A)
  {
    refX=A.refX;
    refY=A.refY;
    refE=A.refE;
    refDx=A.refDx;
  }
  return *this;
}

/// Destructor. Nothing to do since refX, refY, and refE are managed ptr
Histogram1D::~Histogram1D()
{}

} // namespace DataObjects
} // namespace Mantid
