#include "MantidAPI/Dimension.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/IPropertyManager.h"

using Mantid::MDEvents::CoordType;

namespace Mantid
{
namespace API
{

  //-----------------------------------------------------------------------------------------------
  /** Add a new dimension
   *
   * @param dimInfo :: Dimension object which will be copied into the workspace
   */
  void IMDEventWorkspace::addDimension(Dimension dimInfo)
  {
    dimensions.push_back(dimInfo);
  }


  //-----------------------------------------------------------------------------------------------
  /** Get the given dimension
   *
   * @param dim :: index of dimension to set
   * @return Dimension object
   */
  Dimension IMDEventWorkspace::getDimension(size_t dim) const
  {
    return dimensions[dim];
  }


  //-----------------------------------------------------------------------------------------------
  /** Get the index of the dimension that matches the name given
   *
   * @param name :: name of the dimensions
   * @return the index (size_t)
   * @throw runtime_error if it cannot be found.
   */
  size_t IMDEventWorkspace::getDimensionIndexByName(const std::string & name) const
  {
    for (size_t d=0; d<dimensions.size(); d++)
      if (dimensions[d].getName() == name)
        return d;
    throw std::runtime_error("Dimension named '" + name + "' was not found in the IMDEventWorkspace.");
  }


}//namespace MDEvents

}//namespace Mantid






namespace Mantid
{
namespace Kernel
{
  /** In order to be able to cast PropertyWithValue classes correctly a definition for the PropertyWithValue<IMDEventWorkspace> is required */
  template<> DLLExport
  Mantid::API::IMDEventWorkspace_sptr IPropertyManager::getValue<Mantid::API::IMDEventWorkspace_sptr>(const std::string &name) const
  {
    PropertyWithValue<Mantid::API::IMDEventWorkspace_sptr>* prop =
                      dynamic_cast<PropertyWithValue<Mantid::API::IMDEventWorkspace_sptr>*>(getPointerToProperty(name));
    if (prop)
    {
      return *prop;
    }
    else
    {
      std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected IMDEventWorkspace.";
      throw std::runtime_error(message);
    }
  }

  /** In order to be able to cast PropertyWithValue classes correctly a definition for the PropertyWithValue<IMDEventWorkspace> is required */
  template<> DLLExport
  Mantid::API::IMDEventWorkspace_const_sptr IPropertyManager::getValue<Mantid::API::IMDEventWorkspace_const_sptr>(const std::string &name) const
  {
    PropertyWithValue<Mantid::API::IMDEventWorkspace_const_sptr>* prop =
                      dynamic_cast<PropertyWithValue<Mantid::API::IMDEventWorkspace_const_sptr>*>(getPointerToProperty(name));
    if (prop)
    {
      return prop->operator()();
    }
    else
    {
      std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected const IMDEventWorkspace.";
      throw std::runtime_error(message);
    }
  }

} // namespace Kernel
} // namespace Mantid

