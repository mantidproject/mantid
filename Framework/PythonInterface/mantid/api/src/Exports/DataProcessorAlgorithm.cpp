#include "MantidPythonInterface/api/PythonAlgorithm/DataProcessorAdapter.h"
#include "MantidPythonInterface/kernel/Policies/VectorToNumpy.h"
#include "MantidAPI/ITableWorkspace.h"
#include <boost/python/class.hpp>
#include <boost/python/overloads.hpp>

using namespace Mantid::API;
using Mantid::Kernel::Direction;
using Mantid::PythonInterface::DataProcessorAdapter;
using namespace boost::python;

namespace {
typedef Workspace_sptr (DataProcessorAdapter::*loadOverload1)(
    const std::string &);
typedef Workspace_sptr (DataProcessorAdapter::*loadOverload2)(
    const std::string &, const bool);
}

void export_DataProcessorAlgorithm() {
  // for strings will actually create a list
  using Mantid::PythonInterface::Policies::VectorToNumpy;

  class_<DataProcessorAlgorithm, bases<Algorithm>,
         boost::shared_ptr<DataProcessorAdapter>, boost::noncopyable>(
      "DataProcessorAlgorithm", "Base class workflow-type algorithms")

      .def("setLoadAlg", &DataProcessorAdapter::setLoadAlgProxy,
           (arg("self"), arg("alg")),
           "Set the name of the algorithm called using the load() method "
           "[Default=Load]")

      .def("setLoadAlgFileProp", &DataProcessorAdapter::setLoadAlgFilePropProxy,
           (arg("self"), arg("file_prop_name")),
           "Set the name of the file property for the load algorithm when "
           "using "
           "the load() method [Default=Filename]")

      .def("setAccumAlg", &DataProcessorAdapter::setAccumAlgProxy,
           (arg("self"), arg("alg")),
           "Set the name of the algorithm called to accumulate a chunk of "
           "processed data [Default=Plus]")

      .def("copyProperties", &DataProcessorAdapter::copyPropertiesProxy,
           (arg("self"), arg("alg"),
            arg("properties") = boost::python::object(), arg("version") = -1),
           "Copy properties from another algorithm")

      .def("determineChunk", &DataProcessorAdapter::determineChunkProxy,
           (arg("self"), arg("file_name")),
           "Return a TableWorkspace containing the information on how to split "
           "the "
           "input file when processing in chunks")

      .def("loadChunk", &DataProcessorAdapter::loadChunkProxy,
           (arg("self"), arg("row_index")), "Load a chunk of data")

      .def("load", (loadOverload1)&DataProcessorAdapter::loadProxy,
           (arg("self"), arg("input_data")),
           "Loads the given file or workspace data and returns the workspace. "
           "The output is not stored in the AnalysisDataService.")

      .def("load", (loadOverload2)&DataProcessorAdapter::loadProxy,
           (arg("self"), arg("input_data"), arg("load_quite")),
           "Loads the given file or workspace data and returns the workspace. "
           "If loadQuiet=True then output is not stored in the "
           "AnalysisDataService.")

      .def("splitInput", &DataProcessorAdapter::splitInputProxy,
           (arg("self"), arg("input")), return_value_policy<VectorToNumpy>())

      .def("forwardProperties", &DataProcessorAdapter::forwardPropertiesProxy,
           arg("self"))

      .def("getProcessProperties",
           &DataProcessorAdapter::getProcessPropertiesProxy,
           (arg("self"), arg("property_manager")),
           "Returns the named property manager from the service or creates "
           "a new one if it does not exist")

      .def("assemble", &DataProcessorAdapter::assembleProxy,
           (arg("self"), arg("partial_wsname"), arg("output_wsname")),
           "If an MPI build, assemble the partial workspaces from all MPI "
           "processes. "
           "Otherwise, simply returns the input workspace")

      .def("saveNexus", &DataProcessorAdapter::saveNexusProxy,
           (arg("self"), arg("output_wsname"), arg("output_filename")),
           "Save a workspace as a nexus file. If this is an MPI build then "
           "saving only "
           "happens for the main thread.")

      .def("isMainThread", &DataProcessorAdapter::isMainThreadProxy,
           arg("self"),
           "Returns true if this algorithm is the main thread for an MPI "
           "build. For "
           "non-MPI build it always returns true")

      .def("getNThreads", &DataProcessorAdapter::getNThreadsProxy, arg("self"),
           "Returns the number of running MPI processes in an MPI build or 1 "
           "for "
           "a non-MPI build");
}
