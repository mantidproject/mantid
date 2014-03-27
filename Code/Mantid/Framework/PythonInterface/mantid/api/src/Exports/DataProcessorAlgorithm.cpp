#include "MantidPythonInterface/api/PythonAlgorithm/DataProcessorAdapter.h"
#include "MantidPythonInterface/kernel/Policies/VectorToNumpy.h"
#include <boost/python/class.hpp>
#include <boost/python/overloads.hpp>

using namespace Mantid::API;
using Mantid::PythonInterface::DataProcessorAdapter;
using namespace boost::python;

namespace
{
  /// Overload generator for load() method
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(LoadMethodOverload, load, 1, 2)

  typedef Workspace_sptr(DataProcessorAdapter::*loadOverload1)(const std::string&);
  typedef Workspace_sptr(DataProcessorAdapter::*loadOverload2)(const std::string&, const bool);
}

void export_DataProcessorAlgorithm()
{
  // for strings will actually create a list
  using Mantid::PythonInterface::Policies::VectorToNumpy;

  class_<DataProcessorAlgorithm, bases<Algorithm>, boost::shared_ptr<DataProcessorAdapter>,
         boost::noncopyable>("DataProcessorAlgorithm", "Base class workflow-type algorithms")
    .def("setLoadAlg", &DataProcessorAdapter::setLoadAlg,
         "Set the name of the algorithm called using the load() method [Default=Load]")

    .def("setLoadAlgFileProp", &DataProcessorAdapter::setLoadAlgFileProp,
         "Set the name of the file property for the load algorithm when using "
         "the load() method [Default=Filename]")

    .def("setAccumAlg", &DataProcessorAdapter::setAccumAlg,
         "Set the name of the algorithm called to accumulate a chunk of processed data [Default=Plus]")

    .def("determineChunk", &DataProcessorAdapter::determineChunk,
         "Return a TableWorkspace containing the information on how to split the "
         "input file when processing in chunks")

    .def("loadChunk", &DataProcessorAdapter::loadChunk,
         "Load a chunk of data")

    .def("load", (loadOverload1)&DataProcessorAdapter::load,
         "Loads the given file or workspace data and returns the workspace. "
         "The output is not stored in the AnalysisDataService.")

    .def("load", (loadOverload2)&DataProcessorAdapter::load,
         "Loads the given file or workspace data and returns the workspace. "
         "If loadQuiet=True then output is not stored in the AnalysisDataService.")

    .def("splitInput", &DataProcessorAdapter::splitInput,
         return_value_policy<VectorToNumpy>())

    .def("forwardProperties", &DataProcessorAdapter::forwardProperties)

    .def("getProcessProperties", &DataProcessorAdapter::getProcessProperties,
         "Returns the named property manager from the service or creates "
         "a new one if it does not exist")

    .def("assemble", &DataProcessorAdapter::assemble,
         "If an MPI build, assemble the partial workspaces from all MPI processes. "
         "Otherwise, simply returns the input workspace")

    .def("saveNexus", &DataProcessorAdapter::saveNexus,
         "Save a workspace as a nexus file. If this is an MPI build then saving only "
         "happens for the main thread.")

    .def("isMainThread", &DataProcessorAdapter::isMainThread,
         "Returns true if this algorithm is the main thread for an MPI build. For "
         "non-MPI build it always returns true")

    .def("getNThreads", &DataProcessorAdapter::getNThreads,
         "Returns the number of running MPI processes in an MPI build or 1 for "
         "a non-MPI build")
  ;
}

