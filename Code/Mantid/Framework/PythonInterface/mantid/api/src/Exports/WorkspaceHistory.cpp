#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAPI/AlgorithmHistory.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidPythonInterface/kernel/Policies/RemoveConst.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/list.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/self.hpp>

using Mantid::API::WorkspaceHistory;
using Mantid::API::AlgorithmHistory;
using Mantid::API::IAlgorithm;
using namespace boost::python;

namespace Policies = Mantid::PythonInterface::Policies;

/**
 * Return a Python list of history objects from the workspace history as this is
 * far easier to work with than a set
 * @param self :: A reference to the WorkspaceHistory that called this method
 * @returns A python list created from the set of algorithm histories
 */
boost::python::object getHistoriesAsList(WorkspaceHistory& self)
{
  boost::python::list names;
  const auto histories = self.getAlgorithmHistories();
  Mantid::API::AlgorithmHistories::const_iterator itr = histories.begin();
  for(; itr != histories.end(); ++itr)
  {
    names.append(*itr);
  }
  return names;
}

void export_WorkspaceHistory()
{
  register_ptr_to_python<WorkspaceHistory*>();

  class_<WorkspaceHistory, boost::noncopyable>("WorkspaceHistory", no_init)

    .def("getAlgorithmHistories", &getHistoriesAsList,
         "Returns a list of algorithm histories for this workspace history.")
    
    .def("getAlgorithmHistory", &WorkspaceHistory::getAlgorithmHistory, 
          arg("index"),
          return_value_policy<Policies::RemoveConstSharedPtr>(),
         "Returns the algorithm history at the given index in the history")
    
    .def("size", &WorkspaceHistory::size, 
         "Returns the number of algorithms in the immediate history")    

    .def("empty", &WorkspaceHistory::empty, 
         "Returns whether the history has any entries")
    
    .def("lastAlgorithm", &WorkspaceHistory::lastAlgorithm, "Returns the last algorithm run on this workspace so that its properties can be accessed")
    
    .def("getAlgorithm", &WorkspaceHistory::getAlgorithm, "Returns the algorithm at the given index in the history")
    
    // ----------------- Operators --------------------------------------
    .def("__getitem__", &WorkspaceHistory::getAlgorithm)
    .def(self_ns::str(self))
    ;
}

