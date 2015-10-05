#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/MDEvent.h"
#include "MantidDataObjects/MDLeanEvent.h"
#include "MantidPythonInterface/kernel/Registry/RegisterWorkspacePtrToPython.h"

#include <boost/python/class.hpp>

using Mantid::API::IMDEventWorkspace;
using Mantid::DataObjects::MDEventWorkspace;
using Mantid::DataObjects::MDEvent;
using Mantid::DataObjects::MDLeanEvent;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

namespace {

/**
 * @tparam MDE The event type
 * @tparam nd The number of dimensions
 * @param className Name of the class in Python
 */
template <typename MDE, size_t nd>
void MDEventWorkspaceExportImpl(const char *className) {
  typedef MDEventWorkspace<MDE, nd> ExportType;

  class_<ExportType, bases<IMDEventWorkspace>, boost::noncopyable>(className,
                                                                   no_init);

  // register pointers
  RegisterWorkspacePtrToPython<ExportType>();
}
}

void export_MDEventWorkspaces() {
  // The maximum number of dimensions is defined in the MDWorkspaceFactory
  MDEventWorkspaceExportImpl<MDEvent<1>, 1>("MDEventWorkspace1D");
  MDEventWorkspaceExportImpl<MDLeanEvent<1>, 1>("MDLeanEventWorkspace1D");

  MDEventWorkspaceExportImpl<MDEvent<2>, 2>("MDEventWorkspace2D");
  MDEventWorkspaceExportImpl<MDLeanEvent<2>, 2>("MDLeanEventWorkspace2D");

  MDEventWorkspaceExportImpl<MDEvent<3>, 3>("MDEventWorkspace3D");
  MDEventWorkspaceExportImpl<MDLeanEvent<3>, 3>("MDLeanEventWorkspace3D");

  MDEventWorkspaceExportImpl<MDEvent<4>, 4>("MDEventWorkspace4D");
  MDEventWorkspaceExportImpl<MDLeanEvent<4>, 4>("MDLeanEventWorkspace4D");

  MDEventWorkspaceExportImpl<MDEvent<5>, 5>("MDEventWorkspace5D");
  MDEventWorkspaceExportImpl<MDLeanEvent<5>, 5>("MDLeanEventWorkspace5D");

  MDEventWorkspaceExportImpl<MDEvent<6>, 6>("MDEventWorkspace6D");
  MDEventWorkspaceExportImpl<MDLeanEvent<6>, 6>("MDLeanEventWorkspace6D");

  MDEventWorkspaceExportImpl<MDEvent<7>, 7>("MDEventWorkspace7D");
  MDEventWorkspaceExportImpl<MDLeanEvent<7>, 7>("MDLeanEventWorkspace7D");

  MDEventWorkspaceExportImpl<MDEvent<8>, 8>("MDEventWorkspace8D");
  MDEventWorkspaceExportImpl<MDLeanEvent<8>, 8>("MDLeanEventWorkspace8D");

  MDEventWorkspaceExportImpl<MDEvent<9>, 9>("MDEventWorkspace9D");
  MDEventWorkspaceExportImpl<MDLeanEvent<9>, 9>("MDLeanEventWorkspace9D");
}
