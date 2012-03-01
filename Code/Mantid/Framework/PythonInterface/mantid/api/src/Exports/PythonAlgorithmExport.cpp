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
#include <boost/python/overloads.hpp>

using Mantid::API::Algorithm;
using Mantid::PythonInterface::AlgorithmWrapper;
using namespace boost::python;

namespace
{
  // declareProperty(name, defaultValue, validator, doc, direction)
  typedef void(AlgorithmWrapper::*declarePropertyType1)(const std::string &, const boost::python::object &,
                                                        const boost::python::object &, const std::string &, const int);
  // declareProperty(name, defaultValue, doc, direction)
  typedef void(AlgorithmWrapper::*declarePropertyType2)(const std::string &, const boost::python::object &,
                                                        const std::string &, const int);
  // declareProperty(name, defaultValue, direction)
  typedef void(AlgorithmWrapper::*declarePropertyType3)(const std::string &, const boost::python::object &, const int);

  // Overload types
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(declarePropertyType1_Overload, declareProperty, 2, 5);
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(declarePropertyType2_Overload, declareProperty, 3, 4);
}

void export_leaf_classes()
{

  // Export the algorithm wrapper that boost.python makes look like a PythonAlgorithm
  class_<AlgorithmWrapper, bases<Algorithm>, boost::noncopyable>("PythonAlgorithm", "Base class for all Python algorithms")
    .def("declareProperty", (declarePropertyType1)&AlgorithmWrapper::declareProperty,
          declarePropertyType1_Overload(args("name", "defaultValue", "validator=None","doc=''","direction=Direction.Input"),
                                        "Declares a named property where the type is taken from "
                                        "the type of the defaultValue and mapped to an appropriate C++ type"))

    .def("declareProperty", (declarePropertyType2)&AlgorithmWrapper::declareProperty,
         declarePropertyType2_Overload(args("name", "defaultValue", "doc","direction=Direction.Input"),
                                            "Declares a named property where the type is taken from the type "
                                            "of the defaultValue and mapped to an appropriate C++ type"))

    .def("declareProperty", (declarePropertyType3)&AlgorithmWrapper::declareProperty,
         args("name", "defaultValue", "direction"),
         "Declares a named property where the type is taken from the type of "
         "the defaultValue and mapped to an appropriate C++ type")
  ;
}
