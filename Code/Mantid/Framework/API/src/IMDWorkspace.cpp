#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/IPropertyManager.h"

namespace Mantid
{
  namespace API
  {
      IMDWorkspace::~IMDWorkspace()
      {
      }

      /// Get the dimension
      Mantid::Geometry::IMDDimension_const_sptr IMDWorkspace::getDimensionNum(size_t index) const
      {
        if (index==0) return this->getXDimension();
        if (index==1) return this->getYDimension();
        if (index==2) return this->getZDimension();
        if (index==3) return this->getTDimension();
        throw std::runtime_error("IMDWorkspace::getDimensionNum() called with too high of an index.");
      }



      /**
        * Default implementation throws NotImplementedError exception.
        */
      IMDIterator* IMDWorkspace::createIterator() const
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
    PropertyWithValue<Mantid::API::IMDWorkspace_const_sptr>* prop =
                      dynamic_cast<PropertyWithValue<Mantid::API::IMDWorkspace_const_sptr>*>(getPointerToProperty(name));
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

