#ifdef _MSC_VER
#pragma warning(disable : 4250) // Disable warning regarding inheritance via
                                // dominance, we have no way around it with the
                                // design
#endif
#include "MantidAPI/AlgorithmProxy.h"

#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using namespace Mantid::API;
using namespace boost::python;

void export_algorithm_proxy() {

  register_ptr_to_python<boost::shared_ptr<AlgorithmProxy>>();

  // We do not require any additional methods here but some parts of the code
  // specifically check that a proxy has
  // been returned
  class_<AlgorithmProxy, bases<IAlgorithm>, boost::noncopyable>(
      "AlgorithmProxy", "Proxy class returned by managed algorithms", no_init);
}

#ifdef _MSC_VER
#pragma warning(default : 4250)
#endif
