#include <iostream> 
#include "MantidDataObjects/Histogram1D.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/GaussianErrorHelper.h"
#include "MantidAPI/WorkspaceFactory.h"

namespace Mantid
{
namespace DataObjects
{

/*!
 Constructor
 Defaults to use the Gaussian Error Helper
 */
Histogram1D::Histogram1D() : _errorHelper(API::GaussianErrorHelper::Instance())
{}

/*!
 Copy constructor
 \param A :: Histogram to copy
 */
Histogram1D::Histogram1D(const Histogram1D& A) :
  refX(A.refX), refY(A.refY), refE(A.refE), refE2(A.refE2), _errorHelper(A._errorHelper)
{}

/*!
 Assignment operator
 \param A :: Histogram to copy
 \return *this
 */
Histogram1D& Histogram1D::operator=(const Histogram1D& A)
{
  if (this!=&A)
  {
    refX=A.refX;
    refY=A.refY;
    refE=A.refE;
    refE2=A.refE2;
    _errorHelper = A._errorHelper;
//    _spectraNo = A._spectraNo;
  }
  return *this;
}

/// Destructor. Nothing to do since refX, refY, and refE are managed ptr
/// ErrorHelper is a singleton and therefore should not be deleted here
Histogram1D::~Histogram1D()
{}

} // namespace DataObjects
} // namespace Mantid
