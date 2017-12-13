#ifdef _MSC_VER
#pragma warning(disable : 4250) // Disable warning regarding inheritance via
                                // dominance, we have no way around it with the
                                // design
#endif
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidAPI/AlgorithmObserver.h"

#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using namespace Mantid::API;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(AlgorithmObserver)

void export_algorithm_observer() {

  register_ptr_to_python<boost::shared_ptr<AlgorithmObserver>>();

  class_<AlgorithmObserver, boost::noncopyable>(
      "AlgorithmObserver", "Observes Algorithm notifications: start,progress,finish,error.", no_init);
}

#ifdef _MSC_VER
#pragma warning(default : 4250)
#endif
