#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/IPropertyManager.h"

namespace Mantid
{
namespace API
{

// Get a reference to the logger
Kernel::Logger& ITableWorkspace::g_log = Kernel::Logger::get("ITableWorkspace");

void ITableWorkspace::modified()
{
  if (!AnalysisDataService::Instance().doesExist(this->getName())) return;
  Workspace_sptr ws =  AnalysisDataService::Instance().retrieve(this->getName());
  if (!ws) return;
  ITableWorkspace_sptr tws = boost::dynamic_pointer_cast<ITableWorkspace>(ws);
  if (!tws) return;
  AnalysisDataService::Instance().notificationCenter.postNotification(
    new Kernel::DataService<API::Workspace>::AfterReplaceNotification(this->getName(),tws));
}

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
