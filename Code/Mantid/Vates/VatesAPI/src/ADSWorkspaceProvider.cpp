#include "MantidVatesAPI/ADSWorkspaceProvider.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/Exception.h"

using namespace Mantid::API;

namespace Mantid
{
  namespace VATES
  {
      template<typename Workspace_Type>
      ADSWorkspaceProvider<Workspace_Type>::ADSWorkspaceProvider()
      {
      }

      template<typename Workspace_Type>
      ADSWorkspaceProvider<Workspace_Type>::~ADSWorkspaceProvider()
      {
      }

      template<typename Workspace_Type>
      bool ADSWorkspaceProvider<Workspace_Type>::canProvideWorkspace(std::string wsName) const
      {
        bool bCanProvide = false;
        try
        {
          bCanProvide = (NULL != boost::dynamic_pointer_cast<Workspace_Type>(AnalysisDataService::Instance().retrieve(wsName)));
        }
        catch(Mantid::Kernel::Exception::NotFoundError&)
        {
          bCanProvide = false;
        }
        return bCanProvide;
      }

      template<typename Workspace_Type>
      Mantid::API::Workspace_sptr ADSWorkspaceProvider<Workspace_Type>::fetchWorkspace(std::string wsName) const
      {
        return boost::dynamic_pointer_cast<Workspace_Type>(AnalysisDataService::Instance().retrieve(wsName));
      }

      template<typename Workspace_Type>
      void ADSWorkspaceProvider<Workspace_Type>::disposeWorkspace(std::string wsName) const
      {
        AnalysisDataService::Instance().remove(wsName);
      }

      //Templated assembled types.
      template class ADSWorkspaceProvider<Mantid::API::IMDWorkspace>;
      template class ADSWorkspaceProvider<Mantid::API::IMDEventWorkspace>;

  }
}
