#include "MantidAPI/AlgorithmHistory.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidKernel/PropertyHistory.h"
#include "MantidPythonInterface/kernel/Policies/RemoveConst.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/list.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/return_internal_reference.hpp>
#include <boost/python/self.hpp>

using Mantid::API::AlgorithmHistory;
using namespace boost::python;

/**
 * Return a Python list of child history objects from the history as this is
 * far easier to work with than a set
 * @param self :: A reference to the AlgorithmHistory that called this method
 * @returns A python list created from the set of child algorithm histories
 */
boost::python::object
getChildrenAsList(boost::shared_ptr<AlgorithmHistory> self) {
  boost::python::list names;
  const auto histories = self->getChildHistories();
  for (const auto &historie : histories) {
    names.append(historie);
  }
  return names;
}

/**
 * Return a Python list of property history objects from the history
 *
 * @param self :: A reference to the AlgorithmHistory that called this method
 * @returns A python list created from the set of property histories
 */
boost::python::object getPropertiesAsList(AlgorithmHistory &self) {
  boost::python::list names;
  const auto &histories = self.getProperties();
  for (const auto &historie : histories) {
    names.append(historie);
  }
  return names;
}

void export_AlgorithmHistory() {
  register_ptr_to_python<Mantid::API::AlgorithmHistory_sptr>();

  class_<AlgorithmHistory>("AlgorithmHistory", no_init)
      .def("name", &AlgorithmHistory::name, arg("self"),
           return_value_policy<copy_const_reference>(),
           "Returns the name of the algorithm.")

      .def("version", &AlgorithmHistory::version, arg("self"),
           return_value_policy<copy_const_reference>(),
           "Returns the version of the algorithm.")

      .def("executionDuration", &AlgorithmHistory::executionDuration,
           arg("self"), "Returns the execution duration of the algorithm.")

      .def("executionDate", &AlgorithmHistory::executionDate, arg("self"),
           "Returns the execution date of the algorithm.")

      .def("execCount", &AlgorithmHistory::execCount, arg("self"),
           return_value_policy<copy_const_reference>(),
           "Returns the execution number of the algorithm.")

      .def("childHistorySize", &AlgorithmHistory::childHistorySize, arg("self"),
           "Returns the number of the child algorithms.")

      .def("getChildAlgorithmHistory",
           &AlgorithmHistory::getChildAlgorithmHistory,
           (arg("self"), arg("index")),
           "Returns the child algorithm at the given index in the history")

      .def("getChildHistories", &getChildrenAsList, arg("self"),
           "Returns a list of child "
           "algorithm histories for "
           "this algorithm history.")

      .def("getProperties", &getPropertiesAsList, arg("self"),
           "Returns properties for this algorithm history.")

      .def("getPropertyValue", &AlgorithmHistory::getPropertyValue,
           (arg("self"), arg("index")),
           return_value_policy<copy_const_reference>(),
           "Returns the string representation of a specified property.")

      .def("getChildAlgorithm", &AlgorithmHistory::getChildAlgorithm,
           (arg("self"), arg("index")),
           "Returns the algorithm at the given index in the history")
      // ----------------- Operators --------------------------------------
      .def(self_ns::str(self));
}
