#include "MantidDataObjects/Workspace1D.h"

DECLARE_WORKSPACE(Workspace1D)

namespace Mantid
{
namespace DataObjects
{

Workspace1D::Workspace1D() : API::Workspace(), 
			     Histogram1D()
  /// Constructor
{ }

Workspace1D::Workspace1D(const Workspace1D& A) :
  API::Workspace(A),Histogram1D(A)
  /// copy Constructor
{ }

Workspace1D& 
Workspace1D::operator=(const Workspace1D& A)
  /*!
    Assignment operator
    \param A :: Workspace  to copy
    \return *this
   */
{
  if (this!=&A)
    {
      API::Workspace::operator=(A);
      Histogram1D::operator=(A);
    }
  return *this;
}

Workspace1D::~Workspace1D()
  /// Destructor
{}

} // namespace DataObjects
} //NamespaceMantid
