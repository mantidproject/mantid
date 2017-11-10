#include "MantidKernel/PropertyWithValue.h"
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/IMaskWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/ISplittersWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/Workspace.h"

// PropertyWithValue implementation
#include "MantidKernel/PropertyWithValue.tcc"

namespace Mantid {
namespace Kernel {

/// @cond
template class MANTID_API_DLL
    PropertyWithValue<boost::shared_ptr<API::IAlgorithm>>;
template class MANTID_API_DLL
    PropertyWithValue<boost::shared_ptr<API::IEventWorkspace>>;
template class MANTID_API_DLL
    PropertyWithValue<boost::shared_ptr<API::IFunction>>;
template class MANTID_API_DLL
    PropertyWithValue<boost::shared_ptr<API::IMaskWorkspace>>;
template class MANTID_API_DLL
    PropertyWithValue<boost::shared_ptr<API::IMDEventWorkspace>>;
template class MANTID_API_DLL
    PropertyWithValue<boost::shared_ptr<API::IMDHistoWorkspace>>;
template class MANTID_API_DLL
    PropertyWithValue<boost::shared_ptr<API::IMDWorkspace>>;
template class MANTID_API_DLL
    PropertyWithValue<boost::shared_ptr<API::IPeaksWorkspace>>;
template class MANTID_API_DLL
    PropertyWithValue<boost::shared_ptr<API::ISplittersWorkspace>>;
template class MANTID_API_DLL
    PropertyWithValue<boost::shared_ptr<API::ITableWorkspace>>;
template class MANTID_API_DLL
    PropertyWithValue<boost::shared_ptr<API::MatrixWorkspace>>;
template class MANTID_API_DLL
    PropertyWithValue<boost::shared_ptr<API::Workspace>>;
template class MANTID_API_DLL
    PropertyWithValue<boost::shared_ptr<API::WorkspaceGroup>>;
template class MANTID_API_DLL
    PropertyWithValue<boost::shared_ptr<API::ExperimentInfo>>;
/// @endcond

} // namespace Kernel
} // namespace Mantid
