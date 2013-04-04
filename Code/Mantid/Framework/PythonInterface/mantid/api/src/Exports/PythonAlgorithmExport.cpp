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
#include <boost/python/overloads.hpp>

using Mantid::API::Algorithm;
using Mantid::PythonInterface::AlgorithmWrapper;
using Mantid::PythonInterface::PythonAlgorithm;
using Mantid::Kernel::Direction;
using namespace boost::python;

namespace
{
  // declarePyAlgProperty(property*,doc)
  typedef void(PythonAlgorithm::*declarePropertyType1)(Mantid::Kernel::Property*, const std::string &);
  // declarePyAlgProperty(name, defaultValue, validator, doc, direction)
  typedef void(PythonAlgorithm::*declarePropertyType2)(const std::string &, const boost::python::object &,
                                                        const boost::python::object &, const std::string &, const int);
  // declarePyAlgProperty(name, defaultValue, doc, direction)
  typedef void(PythonAlgorithm::*declarePropertyType3)(const std::string &, const boost::python::object &,
                                                        const std::string &, const int);
  // declarePyAlgProperty(name, defaultValue, direction)
  typedef void(PythonAlgorithm::*declarePropertyType4)(const std::string &, const boost::python::object &, const int);

  // Overload types
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(declarePropertyType1_Overload, declarePyAlgProperty, 1, 2);
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(declarePropertyType2_Overload, declarePyAlgProperty, 2, 5);
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(declarePropertyType3_Overload, declarePyAlgProperty, 3, 4);
}

void export_leaf_classes()
{

  // Export PythonAlgorithm but the actual held type in Python is boost::shared_ptr<AlgorithmWrapper>
  // See http://wiki.python.org/moin/boost.python/HowTo#ownership_of_C.2B-.2B-_object_extended_in_Python

  class_<PythonAlgorithm, bases<Algorithm>, boost::shared_ptr<AlgorithmWrapper>,
         boost::noncopyable>("PythonAlgorithm", "Base class for all Python algorithms")
    .def("declareProperty", (declarePropertyType1)&PythonAlgorithm::declarePyAlgProperty,
          declarePropertyType1_Overload((arg("prop"), arg("doc") = "")))

    .def("declareProperty", (declarePropertyType2)&PythonAlgorithm::declarePyAlgProperty,
          declarePropertyType2_Overload((arg("name"), arg("defaultValue"), arg("validator")=object(), arg("doc")="",arg("direction")=Direction::Input),
                                        "Declares a named property where the type is taken from "
                                        "the type of the defaultValue and mapped to an appropriate C++ type"))

    .def("declareProperty", (declarePropertyType3)&PythonAlgorithm::declarePyAlgProperty,
         declarePropertyType3_Overload((arg("name"), arg("defaultValue"), arg("doc")="",arg("direction")=Direction::Input),
                                       "Declares a named property where the type is taken from the type "
                                       "of the defaultValue and mapped to an appropriate C++ type"))

    .def("declareProperty", (declarePropertyType4)&PythonAlgorithm::declarePyAlgProperty,
        (arg("name"), arg("defaultValue"), arg("direction")=Direction::Input),
         "Declares a named property where the type is taken from the type "
         "of the defaultValue and mapped to an appropriate C++ type")

    .def("getLogger", &PythonAlgorithm::getLogger, return_value_policy<reference_existing_object>(),
         "Returns a reference to this algorithm's logger")
    .def("log", &PythonAlgorithm::getLogger, return_value_policy<reference_existing_object>(),
         "Returns a reference to this algorithm's logger") // Traditional name
  ;
}
