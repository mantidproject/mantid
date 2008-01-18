#include "MantidDataObjects/Workspace1D.h"
#include "MantidAPI/TripleRef.h"
#include "MantidAPI/TripleIterator.h"
#include "MantidAPI/TripleIteratorCode.h"
#include "MantidAPI/WorkspaceProperty.h"

DECLARE_WORKSPACE(Workspace1D)

namespace Mantid
{
namespace DataObjects
{

/// Constructor
Workspace1D::Workspace1D() : API::Workspace(), 
			     Histogram1D()
{ }

/// Copy Constructor
Workspace1D::Workspace1D(const Workspace1D& A) :
  API::Workspace(A),Histogram1D(A)
{ }

/*!
    Assignment operator
    \param A :: Workspace  to copy
    \return *this
   */
Workspace1D& Workspace1D::operator=(const Workspace1D& A)
{
  if (this!=&A)
    {
      API::Workspace::operator=(A);
      Histogram1D::operator=(A);
    }
  return *this;
}

/// Destructor
Workspace1D::~Workspace1D()
{}

/** Returns the size of the workspace
 * \returns The number of items the workspace contains
 */
int Workspace1D::size() const
{
  return Histogram1D::size();
}

///get the size of each vector
int Workspace1D::blocksize() const
{
  int retVal = 1000000000;
  //if not empty
  if (size() > 0)
  {
    //set the reteurn value to the length of the first vector
    retVal = size();
  }
  return retVal; 
}

} // namespace DataObjects
} // namespace Mantid

///\cond TEMPLATE
template DLLExport class Mantid::API::triple_iterator<Mantid::API::TripleRef<double>, Mantid::DataObjects::Workspace1D>;
template DLLExport class Mantid::API::triple_iterator<const Mantid::API::TripleRef<double>,Mantid::DataObjects::Workspace1D>;

template DLLExport class Mantid::API::WorkspaceProperty<Mantid::DataObjects::Workspace1D>;

namespace Mantid
{
namespace Kernel
{
template<> DLLExport
Mantid::DataObjects::Workspace1D* PropertyManager::getValue<Mantid::DataObjects::Workspace1D*>(const std::string &name) const
{
  PropertyWithValue<Mantid::DataObjects::Workspace1D*> *prop = 
          dynamic_cast<PropertyWithValue<Mantid::DataObjects::Workspace1D*>*>(getPointerToProperty(name));
  if (prop)
  {
    return *prop;
  }
  else
  {
    throw std::runtime_error("Attempt to assign property of incorrect type");
  }
}
template<> DLLExport
Mantid::DataObjects::Workspace1D_sptr PropertyManager::getValue<Mantid::DataObjects::Workspace1D_sptr>(const std::string &name) const
{
  PropertyWithValue<Mantid::DataObjects::Workspace1D_sptr>* prop = 
                    dynamic_cast<PropertyWithValue<Mantid::DataObjects::Workspace1D_sptr>*>(getPointerToProperty(name));
  if (prop)
  {
    return *prop;
  }
  else
  {
    throw std::runtime_error("Attempt to assign property of incorrect type");
  }
}

} // namespace Kernel
} // namespace Mantid

///\endcond TEMPLATE
