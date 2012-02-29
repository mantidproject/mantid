#ifdef _MSC_VER
  #pragma warning( disable: 4250 ) // Disable warning regarding inheritance via dominance, we have no way around it with the design
#endif
#include "MantidPythonInterface/api/PythonAlgorithm/AlgorithmWrapper.h"
#ifdef _MSC_VER
  #pragma warning( default: 4250 )
#endif

#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/bases.hpp>
#include <boost/python/args.hpp>

using Mantid::API::Algorithm;
using Mantid::PythonInterface::AlgorithmWrapper;
using namespace boost::python;

void export_leaf_classes()
{
  // Function pointer to pick out correct declareProperty
  typedef void(AlgorithmWrapper::*declarePropertyOverload1)(const std::string &, const boost::python::object &, const int);

  /**
   * Export the algorithm wrapper that boost.python makes look like a PythonAlgorithm
   */
  class_<AlgorithmWrapper, bases<Algorithm>, boost::noncopyable>("PythonAlgorithm", "Base class for all Python algorithms")
    .def("declareProperty", (declarePropertyOverload1)&AlgorithmWrapper::declareProperty, args("name", "defaultValue", "direction"),
         "Declares a named property where the type is taken from the type of the defaultValue and mapped to an appropriate C++ type")
    ;
  ;
}
