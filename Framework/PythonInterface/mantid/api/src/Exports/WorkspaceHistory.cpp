// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAPI/AlgorithmHistory.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include "MantidPythonInterface/core/Policies/RemoveConst.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/list.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/self.hpp>

using Mantid::API::WorkspaceHistory;
using namespace boost::python;
namespace Policies = Mantid::PythonInterface::Policies;

GET_POINTER_SPECIALIZATION(WorkspaceHistory)

/**
 * Return a Python list of history objects from the workspace history as this is
 * far easier to work with than a set
 * @param self :: A reference to the WorkspaceHistory that called this method
 * @returns A python list created from the set of algorithm histories
 */
boost::python::list getHistoriesAsList(WorkspaceHistory &self) {
  boost::python::list names;
  const auto &histories = self.getAlgorithmHistories();
  for (const auto &historie : histories) {
    names.append(historie);
  }
  return names;
}

void export_WorkspaceHistory() {
  register_ptr_to_python<WorkspaceHistory *>();

  class_<WorkspaceHistory, boost::noncopyable>("WorkspaceHistory", no_init)

      .def("getAlgorithmHistories", &getHistoriesAsList, arg("self"),
           "Returns a list of algorithm histories for this workspace history.")

      .def("getAlgorithmHistory", &WorkspaceHistory::getAlgorithmHistory,
           (arg("self"), arg("index")),
           return_value_policy<Policies::RemoveConstSharedPtr>(),
           "Returns the algorithm history at the given index in the history")

      .def("size", &WorkspaceHistory::size, arg("self"),
           "Returns the number of algorithms in the immediate history")

      .def("empty", &WorkspaceHistory::empty, arg("self"),
           "Returns whether the history has any entries")

      .def("lastAlgorithm", &WorkspaceHistory::lastAlgorithm, arg("self"),
           "Returns the last algorithm run on this workspace so that its "
           "properties can be accessed")

      .def("getAlgorithm", &WorkspaceHistory::getAlgorithm,
           (arg("self"), arg("index")),
           "Returns the algorithm at the given index in the history")

      // ----------------- Operators --------------------------------------
      .def("__getitem__", &WorkspaceHistory::getAlgorithm,
           (arg("self"), arg("index")),
           "Create an algorithm from a history record at a given index")
      .def(self_ns::str(self));
}
