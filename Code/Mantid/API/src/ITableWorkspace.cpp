#include "MantidAPI/ITableWorkspace.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/IPropertyManager.h"

namespace Mantid
{
namespace API
{

// Get a reference to the logger
Kernel::Logger& ITableWorkspace::g_log = Kernel::Logger::get("ITableWorkspace");

} // namespace API
} // Namespace Mantid

///\cond TEMPLATE
namespace Mantid
{
  namespace Kernel
  {
    template<> DLLExport
    API::ITableWorkspace_sptr IPropertyManager::getValue<API::ITableWorkspace_sptr>(const std::string &name) const
    {
      PropertyWithValue<API::ITableWorkspace_sptr>* prop =
        dynamic_cast<PropertyWithValue<API::ITableWorkspace_sptr>*>(getPointerToProperty(name));
      if (prop)
      {
        return *prop;
      }
      else
      {
        std::string message = "Attempt to assign property "+ name +" to incorrect type (ITableWorkspace)";
        throw std::runtime_error(message);
      }
    }

  } // namespace Kernel
} // namespace Mantid

///\endcond TEMPLATE
