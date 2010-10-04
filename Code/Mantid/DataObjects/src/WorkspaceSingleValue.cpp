#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidAPI/LocatedDataRef.h"
#include "MantidAPI/WorkspaceIterator.h"
#include "MantidAPI/WorkspaceIteratorCode.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceFactory.h"

DECLARE_WORKSPACE(WorkspaceSingleValue)

namespace Mantid
{
  namespace DataObjects
  {

    // Get a reference to the logger
    Kernel::Logger& WorkspaceSingleValue::g_log = Kernel::Logger::get("WorkspaceSingleValue");

    /// Constructor
    WorkspaceSingleValue::WorkspaceSingleValue(double value,double error) :
        API::MatrixWorkspace(),
        _X(1,0),_Y(1,value),_E(1,error)
    { }

    /// Destructor
    WorkspaceSingleValue::~WorkspaceSingleValue()
    {}

    /** Does nothing in this case
    *  @param NVectors This value can only be equal to one, otherwise exception is thrown
    *  @param XLength The number of X data points/bin boundaries
    *  @param YLength The number of data/error points
    */
    void WorkspaceSingleValue::init(const int &NVectors, const int &XLength, const int &YLength)
    {
      (void) NVectors; (void) XLength; (void) YLength; //Avoid compiler warning
    }

    Kernel::cow_ptr<MantidVec> WorkspaceSingleValue::refX(const int index) const
    {
      (void) index; //Avoid compiler warning
      Kernel::cow_ptr<MantidVec> ret;
      ret.access() = _X;
      return ret;
    }
    
  } // namespace DataObjects
} // namespace Mantid

///\cond TEMPLATE
template DLLExport class Mantid::API::workspace_iterator<Mantid::API::LocatedDataRef, Mantid::DataObjects::WorkspaceSingleValue>;
template DLLExport class Mantid::API::workspace_iterator<const Mantid::API::LocatedDataRef, const Mantid::DataObjects::WorkspaceSingleValue>;

template DLLExport class Mantid::API::WorkspaceProperty<Mantid::DataObjects::WorkspaceSingleValue>;

namespace Mantid
{
namespace Kernel
{
    template<> DLLExport
      Mantid::DataObjects::WorkspaceSingleValue_sptr IPropertyManager::getValue<Mantid::DataObjects::WorkspaceSingleValue_sptr>(const std::string &name) const
    {
      PropertyWithValue<Mantid::DataObjects::WorkspaceSingleValue_sptr>* prop =
        dynamic_cast<PropertyWithValue<Mantid::DataObjects::WorkspaceSingleValue_sptr>*>(getPointerToProperty(name));
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
