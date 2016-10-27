#include "MantidAPI/DllConfig.h"
#include "MantidKernel/PropertyWithValue.h"

// PropertyWithValue implementation
#include "MantidKernel/PropertyWithValue.tcc"

namespace Mantid {
namespace API {
class ExperimentInfo;
class IAlgorithm;
class IEventWorkspace;
class IFunction;
class IMaskWorkspace;
class IMDEventWorkspace;
class IMDHistoWorkspace;
class IMDWorkspace;
class IPeaksWorkspace;
class ISplittersWorkspace;
class ITableWorkspace;
class IValidator;
class MatrixWorkspace;
class Workspace;
class WorkspaceGroup;
}
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
    PropertyWithValue<boost::shared_ptr<API::IValidator>>;
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
