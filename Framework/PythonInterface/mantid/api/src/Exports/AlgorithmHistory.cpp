#include "MantidAPI/AlgorithmHistory.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidKernel/PropertyHistory.h"
#include "MantidPythonInterface/kernel/Policies/RemoveConst.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/return_internal_reference.hpp>
#include <boost/python/list.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/self.hpp>

using Mantid::API::AlgorithmHistory;
using Mantid::Kernel::PropertyHistory;
using Mantid::API::IAlgorithm;
using namespace boost::python;

namespace Policies = Mantid::PythonInterface::Policies;

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
  Mantid::API::AlgorithmHistories::const_iterator itr = histories.begin();
  for (; itr != histories.end(); ++itr) {
    names.append(*itr);
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
  const auto histories = self.getProperties();
  std::vector<Mantid::Kernel::PropertyHistory_sptr>::const_iterator iend =
      histories.end();
  for (std::vector<Mantid::Kernel::PropertyHistory_sptr>::const_iterator itr =
           histories.begin();
       itr != iend; ++itr) {
    names.append(*itr);
  }
  return names;
}

void export_AlgorithmHistory() {
  register_ptr_to_python<Mantid::API::AlgorithmHistory_sptr>();

  class_<AlgorithmHistory>("AlgorithmHistory", no_init)
      .def("name", &AlgorithmHistory::name,
           return_value_policy<copy_const_reference>(),
           "Returns the name of the algorithm.")

      .def("version", &AlgorithmHistory::version,
           return_value_policy<copy_const_reference>(),
           "Returns the version of the algorithm.")

      .def("executionDuration", &AlgorithmHistory::executionDuration,
           "Returns the execution duration of the algorithm.")

      .def("executionDate", &AlgorithmHistory::executionDate,
           "Returns the execution date of the algorithm.")

      .def("execCount", &AlgorithmHistory::execCount,
           return_value_policy<copy_const_reference>(),
           "Returns the execution number of the algorithm.")

      .def("childHistorySize", &AlgorithmHistory::childHistorySize,
           "Returns the number of the child algorithms.")

      .def("getChildAlgorithmHistory",
           &AlgorithmHistory::getChildAlgorithmHistory, arg("index"),
           "Returns the child algorithm at the given index in the history")

      .def("getChildHistories", &getChildrenAsList, "Returns a list of child "
                                                    "algorithm histories for "
                                                    "this algorithm history.")

      .def("getProperties", &getPropertiesAsList,
           "Returns properties for this algorithm history.")

      .def("getChildAlgorithm", &AlgorithmHistory::getChildAlgorithm,
           arg("index"),
           "Returns the algorithm at the given index in the history")
      // ----------------- Operators --------------------------------------
      .def(self_ns::str(self));
}
