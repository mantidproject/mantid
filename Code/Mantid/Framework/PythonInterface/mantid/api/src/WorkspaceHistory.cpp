#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAPI/IAlgorithm.h"
#include <boost/python/class.hpp>
#include <boost/python/self.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::API::WorkspaceHistory;
using Mantid::API::IAlgorithm;
using namespace boost::python;

void export_WorkspaceHistory()
{
  register_ptr_to_python<WorkspaceHistory*>();

  class_<WorkspaceHistory, boost::noncopyable>("WorkspaceHistory", no_init)
    .def("lastAlgorithm", &WorkspaceHistory::lastAlgorithm, "Returns the last algorithm run on this workspace so that its properties can be accessed")
    .def("getAlgorithm", &WorkspaceHistory::getAlgorithm, "Returns the algorithm at the given index in the history")
    // ----------------- Operators --------------------------------------
    .def("__getitem__", &WorkspaceHistory::getAlgorithm)
    .def(self_ns::str(self))
    ;
}

