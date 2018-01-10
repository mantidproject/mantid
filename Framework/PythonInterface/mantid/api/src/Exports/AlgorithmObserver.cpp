#ifdef _MSC_VER
#pragma warning(disable : 4250) // Disable warning regarding inheritance via
                                // dominance, we have no way around it with the
                                // design
#endif
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidPythonInterface/api/Algorithms/AlgorithmObserverAdapter.h"

#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <iostream>

using namespace Mantid::API;
using namespace Mantid::PythonInterface;
using namespace boost::python;

void observeStart(AlgorithmObserver &self,
                             boost::python::object alg) {
  IAlgorithm_sptr& calg = boost::python::extract<IAlgorithm_sptr&>(alg);
  self.observeStart(calg);
}

GET_POINTER_SPECIALIZATION(AlgorithmObserver)

void export_algorithm_observer() {

  register_ptr_to_python<boost::shared_ptr<AlgorithmObserver>>();

  class_<AlgorithmObserver, bases<>, boost::shared_ptr<AlgorithmObserverAdapter>, boost::noncopyable>(
      "AlgorithmObserver",
      "Observes Algorithm notifications: start,progress,finish,error.")
      .def("observeStart", &observeStart,
           (arg("self"), arg("alg")), "Observe algorithm for its start notification.")
      .def("observeStarting", &AlgorithmObserver::observeStarting,
           arg("self"), "Observe the AlgorithmManager for starting algorithms.")
      ;
}

#ifdef _MSC_VER
#pragma warning(default : 4250)
#endif
