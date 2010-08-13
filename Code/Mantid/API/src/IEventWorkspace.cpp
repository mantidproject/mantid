//------------------------------------------------------
// Includes
//------------------------------------------------------
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/LocatedDataRef.h"
#include "MantidAPI/WorkspaceIterator.h"
#include "MantidAPI/WorkspaceIteratorCode.h"

///\cond TEMPLATE
template DLLExport class Mantid::API::workspace_iterator<Mantid::API::LocatedDataRef,Mantid::API::IEventWorkspace>;
template DLLExport class Mantid::API::workspace_iterator<const Mantid::API::LocatedDataRef, const Mantid::API::IEventWorkspace>;

/*
 * In order to be able to cast PropertyWithValue classes correctly a definition for the PropertyWithValue<IEventWorkspace> is required 
 *
 */
namespace Mantid
{
namespace Kernel
{

template<> DLLExport
Mantid::API::IEventWorkspace_sptr IPropertyManager::getValue<Mantid::API::IEventWorkspace_sptr>(const std::string &name) const
{
  PropertyWithValue<Mantid::API::IEventWorkspace_sptr>* prop =
                    dynamic_cast<PropertyWithValue<Mantid::API::IEventWorkspace_sptr>*>(getPointerToProperty(name));
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

template<> DLLExport
Mantid::API::IEventWorkspace_const_sptr IPropertyManager::getValue<Mantid::API::IEventWorkspace_const_sptr>(const std::string &name) const
{
  PropertyWithValue<Mantid::API::IEventWorkspace_sptr>* prop =
                    dynamic_cast<PropertyWithValue<Mantid::API::IEventWorkspace_sptr>*>(getPointerToProperty(name));
  if (prop)
  {
    return prop->operator()();
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
