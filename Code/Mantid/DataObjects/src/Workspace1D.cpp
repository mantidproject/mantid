#include "MantidDataObjects/Workspace1D.h"
#include "MantidAPI/LocatedDataRef.h"
#include "MantidAPI/WorkspaceIterator.h"
#include "MantidAPI/WorkspaceIteratorCode.h"
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
    /** Sets the size of the workspace and initializes arrays to zero
    *  @param NVectors This value can only be equal to one, otherwise exception is thrown
    *  @param XLength The number of X data points/bin boundaries 
    *  @param YLength The number of data/error points 
    */
    void Workspace1D::init(const int &NVectors, const int &XLength, const int &YLength)
    {
      // Doesn't set the size of the X/Y/E vectors at present. May want to later.

      if(NVectors > 1)
        throw std::invalid_argument("Workspace1D::init() cannot create a workspace1D with Nvectors > 1");
      Histogram1D::RCtype t1,t2;
      t1.access().resize(XLength);//this call initializes array to zero  
      t2.access().resize(YLength);
      this->setX(t1);
      // Y,E,E2 arrays populated
      this->setData(t2,t2,t2);
    }

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
template DLLExport class Mantid::API::workspace_iterator<Mantid::API::LocatedDataRef, Mantid::DataObjects::Workspace1D>;
template DLLExport class Mantid::API::workspace_iterator<const Mantid::API::LocatedDataRef, const Mantid::DataObjects::Workspace1D>;

template DLLExport class Mantid::API::WorkspaceProperty<Mantid::DataObjects::Workspace1D>;

namespace Mantid
{
namespace Kernel
{
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
