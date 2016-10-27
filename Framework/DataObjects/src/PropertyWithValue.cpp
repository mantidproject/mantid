#include "MantidDataObjects/DllConfig.h"
#include "MantidKernel/PropertyWithValue.h"

// PropertyWithValue implementation
#include "MantidKernel/PropertyWithValue.tcc"

namespace Mantid {
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

template <size_t nd> class MDEvent;
template <size_t nd> class MDLeanEvent;
template <class MDE, size_t nd> class MDEventWorkspace;

template <size_t nd> using MDEventWS = MDEventWorkspace<MDEvent<nd>, nd>;
template <size_t nd>
using MDLeanEventWS = MDEventWorkspace<MDLeanEvent<nd>, nd>;
}
namespace Kernel {

/// @cond
template class MANTID_DATAOBJECTS_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::EventWorkspace>>;
template class MANTID_DATAOBJECTS_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::GroupingWorkspace>>;
template class MANTID_DATAOBJECTS_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MaskWorkspace>>;
template class MANTID_DATAOBJECTS_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDEventWS<1>>>;
template class MANTID_DATAOBJECTS_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDEventWS<2>>>;
template class MANTID_DATAOBJECTS_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDEventWS<3>>>;
template class MANTID_DATAOBJECTS_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDEventWS<4>>>;
template class MANTID_DATAOBJECTS_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDEventWS<5>>>;
template class MANTID_DATAOBJECTS_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDEventWS<6>>>;
template class MANTID_DATAOBJECTS_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDEventWS<7>>>;
template class MANTID_DATAOBJECTS_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDEventWS<8>>>;
template class MANTID_DATAOBJECTS_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDEventWS<9>>>;
template class MANTID_DATAOBJECTS_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDEventWS<10>>>;
template class MANTID_DATAOBJECTS_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDLeanEventWS<1>>>;
template class MANTID_DATAOBJECTS_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDLeanEventWS<2>>>;
template class MANTID_DATAOBJECTS_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDLeanEventWS<3>>>;
template class MANTID_DATAOBJECTS_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDLeanEventWS<4>>>;
template class MANTID_DATAOBJECTS_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDLeanEventWS<5>>>;
template class MANTID_DATAOBJECTS_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDLeanEventWS<6>>>;
template class MANTID_DATAOBJECTS_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDLeanEventWS<7>>>;
template class MANTID_DATAOBJECTS_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDLeanEventWS<8>>>;
template class MANTID_DATAOBJECTS_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDLeanEventWS<9>>>;
template class MANTID_DATAOBJECTS_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDLeanEventWS<10>>>;
template class MANTID_DATAOBJECTS_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::MDHistoWorkspace>>;
template class MANTID_DATAOBJECTS_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::OffsetsWorkspace>>;
template class MANTID_DATAOBJECTS_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::PeaksWorkspace>>;
template class MANTID_DATAOBJECTS_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::RebinnedOutput>>;
template class MANTID_DATAOBJECTS_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::SpecialWorkspace2D>>;
template class MANTID_DATAOBJECTS_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::SplittersWorkspace>>;
template class MANTID_DATAOBJECTS_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::TableWorkspace>>;
template class MANTID_DATAOBJECTS_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::Workspace2D>>;
template class MANTID_DATAOBJECTS_DLL
    PropertyWithValue<boost::shared_ptr<DataObjects::WorkspaceSingleValue>>;
/// @endcond

} // namespace Kernel
} // namespace Mantid
