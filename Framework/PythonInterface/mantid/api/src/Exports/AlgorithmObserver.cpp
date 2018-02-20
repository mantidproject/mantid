#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidPythonInterface/api/Algorithms/AlgorithmObserverAdapter.h"

#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <iostream>

using namespace Mantid::API;
using namespace Mantid::PythonInterface;
using namespace boost::python;

void observeFinish(AlgorithmObserver &self, boost::python::object alg) {
  IAlgorithm_sptr &calg = boost::python::extract<IAlgorithm_sptr &>(alg);
  self.observeFinish(calg);
}

void observeError(AlgorithmObserver &self, boost::python::object alg) {
  IAlgorithm_sptr &calg = boost::python::extract<IAlgorithm_sptr &>(alg);
  self.observeError(calg);
}

void observeProgress(AlgorithmObserver &self, boost::python::object alg) {
  IAlgorithm_sptr &calg = boost::python::extract<IAlgorithm_sptr &>(alg);
  self.observeProgress(calg);
}

GET_POINTER_SPECIALIZATION(AlgorithmObserver)

void export_algorithm_observer() {

  register_ptr_to_python<boost::shared_ptr<AlgorithmObserver>>();

  class_<AlgorithmObserver, bases<>,
         boost::shared_ptr<AlgorithmObserverAdapter>, boost::noncopyable>(
      "AlgorithmObserver",
      "Observes Algorithm notifications: start,progress,finish,error.")
      .def("observeStarting", &AlgorithmObserver::observeStarting, arg("self"),
           "Observe the AlgorithmManager for starting algorithms.")
      .def("observeFinish", &observeFinish, (arg("self"), arg("alg")),
           "Observe algorithm for its finish notification.")
      .def("observeError", &observeError, (arg("self"), arg("alg")),
           "Observe algorithm for its error notification.")
      .def("observeProgress", &observeProgress, (arg("self"), arg("alg")),
           "Observe algorithm for its progress notification.");
}
