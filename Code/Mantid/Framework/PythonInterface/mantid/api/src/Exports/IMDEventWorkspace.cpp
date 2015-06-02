#include "MantidAPI/IMDEventWorkspace.h"

#include "MantidPythonInterface/kernel/Registry/DataItemInterface.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using namespace Mantid::API;
using Mantid::PythonInterface::Registry::DataItemInterface;
using namespace boost::python;

namespace {
// THIS NUMBER SHOULD MATCH MAX_MD_DIMENSIONS_NUM IN
// MDEvents/inc/MantidMDEvents/MDEventFactory
static const unsigned int MAX_MD_DIMS = 9;
static const unsigned int NUM_EVENT_TYPES = 2;
}

void export_IMDEventWorkspace() {
  // MDEventWorkspace class
  class_<IMDEventWorkspace, bases<IMDWorkspace, MultipleExperimentInfos>,
         boost::noncopyable>("IMDEventWorkspace", no_init)
      .def("getNPoints", &IMDEventWorkspace::getNPoints,
           "Returns the total number of points (events) in this workspace")

      .def("getNumDims", &IMDEventWorkspace::getNumDims,
           "Returns the number of dimensions in this workspace")

      .def("getBoxController", (BoxController_sptr (IMDEventWorkspace::*)()) &
                                   IMDEventWorkspace::getBoxController,
           "Returns the BoxController used in this workspace");

  //-----------------------------------------------------------------------------------------------
  DataItemInterface<IMDEventWorkspace> entry;
  // The IDs for the MDEventWorkpaces are constructed from the event types and
  // number of dimensions
  const char *eventTypes[NUM_EVENT_TYPES] = {"MDEvent", "MDLeanEvent"};

  std::ostringstream out;
  for (unsigned int i = 1; i <= MAX_MD_DIMS; ++i) {
    for (unsigned int j = 0; j < NUM_EVENT_TYPES; ++j) {
      out.str("");
      out << "MDEventWorkspace<" << eventTypes[j] << "," << i << ">";
      entry.castFromID(out.str());
    }
  }
}
