// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/ITableWorkspace.h"
#include "MantidPythonInterface/api/PythonAlgorithm/DataProcessorAdapter.h"
#include "MantidPythonInterface/kernel/Policies/VectorToNumpy.h"
#include <boost/python/class.hpp>
#include <boost/python/overloads.hpp>

using namespace Mantid::API;
using Mantid::PythonInterface::DataProcessorAdapter;
using namespace boost::python;

namespace {
template <class Base>
using loadOverload = Workspace_sptr (DataProcessorAdapter<Base>::*)(
    const std::string &, const bool);

template <class Base> void do_export(const std::string &name) {
  // for strings will actually create a list
  using Mantid::PythonInterface::Policies::VectorToNumpy;
  using Adapter = DataProcessorAdapter<Base>;

  class_<GenericDataProcessorAlgorithm<Base>, bases<Base>,
         boost::shared_ptr<Adapter>, boost::noncopyable>(
      name.c_str(), "Base class workflow-type algorithms")

      .def("setLoadAlg", &Adapter::setLoadAlgProxy, (arg("self"), arg("alg")),
           "Set the name of the algorithm called using the load() method "
           "[Default=Load]")

      .def("setLoadAlgFileProp", &Adapter::setLoadAlgFilePropProxy,
           (arg("self"), arg("file_prop_name")),
           "Set the name of the file property for the load algorithm when "
           "using "
           "the load() method [Default=Filename]")

      .def("setAccumAlg", &Adapter::setAccumAlgProxy, (arg("self"), arg("alg")),
           "Set the name of the algorithm called to accumulate a chunk of "
           "processed data [Default=Plus]")

      .def("copyProperties", &Adapter::copyPropertiesProxy,
           (arg("self"), arg("alg"),
            arg("properties") = boost::python::object(), arg("version") = -1),
           "Copy properties from another algorithm")

      .def("determineChunk", &Adapter::determineChunkProxy,
           (arg("self"), arg("file_name")),
           "Return a TableWorkspace containing the information on how to split "
           "the "
           "input file when processing in chunks")

      .def("loadChunk", &Adapter::loadChunkProxy,
           (arg("self"), arg("row_index")), "Load a chunk of data")

      .def("load", (loadOverload<Base>)&Adapter::loadProxy,
           (arg("self"), arg("input_data"), arg("load_quiet") = false),
           "Loads the given file or workspace data and returns the workspace. "
           "If loadQuiet=True then output is not stored in the "
           "AnalysisDataService.")

      .def("splitInput", &Adapter::splitInputProxy, (arg("self"), arg("input")),
           return_value_policy<VectorToNumpy>())

      .def("forwardProperties", &Adapter::forwardPropertiesProxy, arg("self"))

      .def("getProcessProperties", &Adapter::getProcessPropertiesProxy,
           (arg("self"), arg("property_manager")),
           "Returns the named property manager from the service or creates "
           "a new one if it does not exist")

      .def("assemble", &Adapter::assembleProxy,
           (arg("self"), arg("partial_wsname"), arg("output_wsname")),
           "If an MPI build, assemble the partial workspaces from all MPI "
           "processes. "
           "Otherwise, simply returns the input workspace")

      .def("saveNexus", &Adapter::saveNexusProxy,
           (arg("self"), arg("output_wsname"), arg("output_filename")),
           "Save a workspace as a nexus file. If this is an MPI build then "
           "saving only "
           "happens for the main thread.")

      .def("isMainThread", &Adapter::isMainThreadProxy, arg("self"),
           "Returns true if this algorithm is the main thread for an MPI "
           "build. For "
           "non-MPI build it always returns true")

      .def("getNThreads", &Adapter::getNThreadsProxy, arg("self"),
           "Returns the number of running MPI processes in an MPI build or 1 "
           "for "
           "a non-MPI build");
}
} // namespace

void export_DataProcessorAlgorithm() {
  do_export<Algorithm>("DataProcessorAlgorithm");
}
void export_SerialDataProcessorAlgorithm() {
  do_export<SerialAlgorithm>("SerialDataProcessorAlgorithm");
}
void export_ParallelDataProcessorAlgorithm() {
  do_export<ParallelAlgorithm>("ParallelDataProcessorAlgorithm");
}
void export_DistributedDataProcessorAlgorithm() {
  do_export<DistributedAlgorithm>("DistributedDataProcessorAlgorithm");
}
