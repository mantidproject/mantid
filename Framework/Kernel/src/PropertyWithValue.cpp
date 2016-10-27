#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/Matrix.h"

// PropertyWithValue implementation
#include "MantidKernel/PropertyWithValue.tcc"

class WorkspaceTester;
class TableWorkspaceTester;

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
namespace DataObjects {
class EventWorkspace;
class GroupingWorkspace;
class MaskWorkspace;
class MDHistoWorkspace;
class OffsetsWorkspace;
class PeaksWorkspace;
class RebinnedOutput;
class SpecialWorkspace2D;
class SplittersWorkspace;
class TableWorkspace;
class Workspace2D;
class WorkspaceSingleValue;

template <size_t nd>
class MDEvent;
template <size_t nd>
class MDLeanEvent;
template <class MDE, size_t nd>
class MDEventWorkspace;

template <size_t nd> using MDEventWS = MDEventWorkspace<MDEvent<nd>, nd>;
template <size_t nd>
using MDLeanEventWS = MDEventWorkspace<MDLeanEvent<nd>, nd>;
}
namespace Kernel {

#define PROPERTYWITHVALUE_SAVEPROPERTY(type)                                   \
  template <>                                                                  \
  void PropertyWithValue<type>::saveProperty(::NeXus::File *file) {            \
    file->makeGroup(this->name(), "NXlog", 1);                                 \
    file->writeData("value", m_value);                                         \
    file->closeGroup();                                                        \
  }

PROPERTYWITHVALUE_SAVEPROPERTY(float)
PROPERTYWITHVALUE_SAVEPROPERTY(double)
PROPERTYWITHVALUE_SAVEPROPERTY(int32_t)
PROPERTYWITHVALUE_SAVEPROPERTY(uint32_t)
PROPERTYWITHVALUE_SAVEPROPERTY(int64_t)
PROPERTYWITHVALUE_SAVEPROPERTY(uint64_t)
PROPERTYWITHVALUE_SAVEPROPERTY(std::string)
PROPERTYWITHVALUE_SAVEPROPERTY(std::vector<double>)
PROPERTYWITHVALUE_SAVEPROPERTY(std::vector<int32_t>)

/// @cond
template class MANTID_KERNEL_DLL PropertyWithValue<uint16_t>;
template class MANTID_KERNEL_DLL PropertyWithValue<bool>;
template class MANTID_KERNEL_DLL PropertyWithValue<OptionalBool>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<float>>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<uint16_t>>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<uint32_t>>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<int64_t>>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<uint64_t>>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<bool>>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<OptionalBool>>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<std::string>>;
template class MANTID_KERNEL_DLL PropertyWithValue<Matrix<float>>;
template class MANTID_KERNEL_DLL PropertyWithValue<Matrix<double>>;
template class MANTID_KERNEL_DLL PropertyWithValue<Matrix<int>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<std::vector<std::vector<int32_t>>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<std::vector<std::vector<std::string>>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<PropertyManager>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<API::IAlgorithm>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<API::IEventWorkspace>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<API::IFunction>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<API::IMaskWorkspace>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<API::IMDEventWorkspace>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<API::IMDHistoWorkspace>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<API::IMDWorkspace>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<API::IPeaksWorkspace>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<API::ISplittersWorkspace>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<API::ITableWorkspace>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<API::IValidator>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<API::MatrixWorkspace>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<API::Workspace>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<API::WorkspaceGroup>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<WorkspaceTester>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<TableWorkspaceTester>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<API::ExperimentInfo>>;

template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::EventWorkspace>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::GroupingWorkspace>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MaskWorkspace>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDEventWS<1>>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDEventWS<2>>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDEventWS<3>>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDEventWS<4>>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDEventWS<5>>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDEventWS<6>>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDEventWS<7>>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDEventWS<8>>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDEventWS<9>>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDEventWS<10>>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDLeanEventWS<1>>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDLeanEventWS<2>>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDLeanEventWS<3>>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDLeanEventWS<4>>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDLeanEventWS<5>>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDLeanEventWS<6>>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDLeanEventWS<7>>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDLeanEventWS<8>>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDLeanEventWS<9>>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDLeanEventWS<10>>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDHistoWorkspace>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::OffsetsWorkspace>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::PeaksWorkspace>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::RebinnedOutput>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::SpecialWorkspace2D>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::SplittersWorkspace>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::TableWorkspace>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::Workspace2D>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::WorkspaceSingleValue>>;
/// @endcond

// The explicit template instantiations for some types does not have an export
// macro
// since this produces a warning on "gcc: warning: type attributes ignored after
// type is already define". We can remove the issue, by removing the visibility
// attribute
template class PropertyWithValue<float>;
template class PropertyWithValue<double>;
template class PropertyWithValue<int32_t>;
template class PropertyWithValue<uint32_t>;
template class PropertyWithValue<int64_t>;
template class PropertyWithValue<uint64_t>;

template class PropertyWithValue<std::vector<double>>;
template class PropertyWithValue<std::vector<int32_t>>;

template class PropertyWithValue<std::string>;

} // namespace Kernel
} // namespace Mantid
