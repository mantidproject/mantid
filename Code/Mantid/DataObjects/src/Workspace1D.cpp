#include "MantidDataObjects/Workspace1D.h"
#include "MantidAPI/RefAxis.h"
#include "MantidAPI/LocatedDataRef.h"
#include "MantidAPI/WorkspaceIterator.h"
#include "MantidAPI/WorkspaceIteratorCode.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceFactory.h"

namespace Mantid
{
  namespace DataObjects
  {

    DECLARE_WORKSPACE(Workspace1D)

    // Get a reference to the logger
    Kernel::Logger& Workspace1D::g_log = Kernel::Logger::get("Workspace1D");

    /// Constructor
    Workspace1D::Workspace1D() : API::MatrixWorkspace(),
      Histogram1D()
    { }

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
      if(NVectors != 1)
        throw std::out_of_range("Workspace1D::init() cannot create a workspace1D with Nvectors > 1");

      m_axes.resize(1);
      m_axes[0] = new API::RefAxis(XLength, this);

      MantidVecPtr t1,t2;
      t1.access().resize(XLength);//this call initializes array to zero
      t2.access().resize(YLength);
      this->setX(t1);
      // Y,E arrays populated
      this->setData(t2,t2);
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
      Mantid::DataObjects::Workspace1D_sptr IPropertyManager::getValue<Mantid::DataObjects::Workspace1D_sptr>(const std::string &name) const
    {
      PropertyWithValue<Mantid::DataObjects::Workspace1D_sptr>* prop =
        dynamic_cast<PropertyWithValue<Mantid::DataObjects::Workspace1D_sptr>*>(getPointerToProperty(name));
      if (prop)
      {
        return *prop;
      }
      else
      {
        std::string message = "Attempt to assign property "+ name +" to incorrect type";
        throw std::runtime_error(message);
      }
    }

} // namespace Kernel
} // namespace Mantid

///\endcond TEMPLATE
