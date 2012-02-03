#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/IPropertyManager.h"

namespace Mantid
{
  namespace API
  {
      //-----------------------------------------------------------------------------------------------
      /** Default constructor */
      IMDWorkspace::IMDWorkspace()
      : Workspace(),
        Mantid::API::MDGeometry()
      {
      }

      //-----------------------------------------------------------------------------------------------
      /** Copy constructor */
      IMDWorkspace::IMDWorkspace(const IMDWorkspace & other)
      : Workspace(other),
        Mantid::API::MDGeometry(other)
      {
      }

      /// Destructor
      IMDWorkspace::~IMDWorkspace()
      {
      }

      /**
        * Default implementation throws NotImplementedError exception.
        */
      IMDIterator* IMDWorkspace::createIterator(Mantid::Geometry::MDImplicitFunction * ) const
      {
        throw Kernel::Exception::NotImplementedError("Iterator is not implemented for this workspace");
      }
  }
}

namespace Mantid
{
namespace Kernel
{
  /** In order to be able to cast PropertyWithValue classes correctly a definition for the PropertyWithValue<IMDEventWorkspace> is required */
  template<> MANTID_API_DLL
  Mantid::API::IMDWorkspace_sptr IPropertyManager::getValue<Mantid::API::IMDWorkspace_sptr>(const std::string &name) const
  {
    PropertyWithValue<Mantid::API::IMDWorkspace_sptr>* prop =
                      dynamic_cast<PropertyWithValue<Mantid::API::IMDWorkspace_sptr>*>(getPointerToProperty(name));
    if (prop)
    {
      return *prop;
    }
    else
    {
      std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected IMDWorkspace.";
      throw std::runtime_error(message);
    }
  }

  /** In order to be able to cast PropertyWithValue classes correctly a definition for the PropertyWithValue<IMDWorkspace_const_sptr> is required */
  template<> MANTID_API_DLL
  Mantid::API::IMDWorkspace_const_sptr IPropertyManager::getValue<Mantid::API::IMDWorkspace_const_sptr>(const std::string &name) const
  {
    PropertyWithValue<Mantid::API::IMDWorkspace_sptr>* prop =
                      dynamic_cast<PropertyWithValue<Mantid::API::IMDWorkspace_sptr>*>(getPointerToProperty(name));
    if (prop)
    {
      return prop->operator()();
    }
    else
    {
      std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected const IMDWorkspace.";
      throw std::runtime_error(message);
    }
  }

} // namespace Kernel
} // namespace Mantid

