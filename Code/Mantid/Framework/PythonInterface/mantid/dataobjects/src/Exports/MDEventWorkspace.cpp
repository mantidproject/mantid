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
  template<typename MDE, size_t nd>
  void MDEventWorkspaceExportImpl(const char *className) {
    typedef MDEventWorkspace<MDE, nd> ExportType;

    class_<ExportType, bases<IMDEventWorkspace>,
           boost::noncopyable>(className, no_init)
      ;

    // register pointers
    RegisterWorkspacePtrToPython<ExportType>();
  }

}

void export_MDEventWorkspaces()
{
  // Export a type for each type generated in the MDWorkspaceFactory
  MDEventWorkspaceExportImpl<MDEvent<1>, 1>("MDEventWorkspace1D");
  MDEventWorkspaceExportImpl<MDLeanEvent<1>, 1>("MDLeanEventWorkspace1D");

  MDEventWorkspaceExportImpl<MDEvent<2>, 2>("MDEventWorkspace2D");
  MDEventWorkspaceExportImpl<MDLeanEvent<2>, 2>("MDLeanEventWorkspace2D");

  MDEventWorkspaceExportImpl<MDEvent<3>, 3>("MDEventWorkspace3D");
  MDEventWorkspaceExportImpl<MDLeanEvent<3>, 3>("MDLeanEventWorkspace3D");

}
