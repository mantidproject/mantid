#include "MantidVatesAPI/ADSWorkspaceProvider.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/Exception.h"

using namespace Mantid::API;

namespace Mantid {
namespace VATES {

template <typename Workspace_Type>
bool ADSWorkspaceProvider<Workspace_Type>::canProvideWorkspace(
    std::string wsName) const {
  bool bCanProvide = false;
  try {
    bCanProvide =
        (nullptr !=
         AnalysisDataService::Instance().retrieveWS<Workspace_Type>(wsName));
  } catch (Mantid::Kernel::Exception::NotFoundError &) {
    bCanProvide = false;
  }
  return bCanProvide;
}

template <typename Workspace_Type>
Mantid::API::Workspace_sptr
ADSWorkspaceProvider<Workspace_Type>::fetchWorkspace(std::string wsName) const {
  return AnalysisDataService::Instance().retrieveWS<Workspace_Type>(wsName);
}

template <typename Workspace_Type>
void ADSWorkspaceProvider<Workspace_Type>::disposeWorkspace(
    std::string wsName) const {
  AnalysisDataService::Instance().remove(wsName);
}

///@cond
// Templated assembled types.
template class DLLExport ADSWorkspaceProvider<Mantid::API::IMDWorkspace>;
template class DLLExport ADSWorkspaceProvider<Mantid::API::IMDEventWorkspace>;
template class DLLExport ADSWorkspaceProvider<Mantid::API::IMDHistoWorkspace>;
///@endcond
}
}
