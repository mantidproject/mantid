#ifdef _MSC_VER
  #pragma warning( disable: 4250 ) // Disable warning regarding inheritance via dominance, we have no way around it with the design
#endif
#include "MantidPythonInterface/api/PythonAlgorithm/AlgorithmAdapter.h"
#ifdef _MSC_VER
  #pragma warning( default: 4250 )
#endif

#include <boost/python/bases.hpp>
#include <boost/python/class.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/scope.hpp>

using Mantid::API::Algorithm;
using Mantid::PythonInterface::AlgorithmAdapter;
using Mantid::Kernel::Direction;
using namespace boost::python;

namespace
{
  typedef AlgorithmAdapter<Algorithm> PythonAlgorithm;

  // declarePyAlgProperty(property*,doc)
  typedef void(*declarePropertyType1)(boost::python::object & self, Mantid::Kernel::Property*, const std::string &);
  // declarePyAlgProperty(name, defaultValue, validator, doc, direction)
  typedef void(*declarePropertyType2)(boost::python::object & self, const std::string &, const boost::python::object &,
                                                        const boost::python::object &, const std::string &, const int);
  // declarePyAlgProperty(name, defaultValue, doc, direction)
  typedef void(*declarePropertyType3)(boost::python::object & self, const std::string &, const boost::python::object &,
                                                        const std::string &, const int);
  // declarePyAlgProperty(name, defaultValue, direction)
  typedef void(*declarePropertyType4)(boost::python::object & self, const std::string &, const boost::python::object &, const int);

  // Overload types
  BOOST_PYTHON_FUNCTION_OVERLOADS(declarePropertyType1_Overload, PythonAlgorithm::declarePyAlgProperty, 2, 3)
  BOOST_PYTHON_FUNCTION_OVERLOADS(declarePropertyType2_Overload, PythonAlgorithm::declarePyAlgProperty, 3, 6)
  BOOST_PYTHON_FUNCTION_OVERLOADS(declarePropertyType3_Overload, PythonAlgorithm::declarePyAlgProperty, 4, 5)
}

void export_leaf_classes()
{

  register_ptr_to_python<boost::shared_ptr<Algorithm>>();

  // Export Algorithm but the actual held type in Python is boost::shared_ptr<AlgorithmAdapter>
  // See http://wiki.python.org/moin/boost.python/HowTo#ownership_of_C.2B-.2B-_object_extended_in_Python

  class_<Algorithm, bases<Mantid::API::IAlgorithm>, boost::shared_ptr<PythonAlgorithm>,
         boost::noncopyable>("Algorithm", "Base class for all algorithms")
    .def("fromString", &Algorithm::fromString, "Initialize the algorithm from a string representation")
    .staticmethod("fromString")

    .def("createChildAlgorithm", &Algorithm::createChildAlgorithm,
         (arg("name"),arg("startProgress")=-1.0,arg("endProgress")=-1.0,
          arg("enableLogging")=true,arg("version")=-1), "Creates and intializes a named child algorithm. Output workspaces are given a dummy name.")

    .def("declareProperty", (declarePropertyType1)&PythonAlgorithm::declarePyAlgProperty,
          declarePropertyType1_Overload((arg("self"), arg("prop"), arg("doc") = "")))

    .def("enableHistoryRecordingForChild", &Algorithm::enableHistoryRecordingForChild,
          (args("on")), "Turns history recording on or off for an algorithm.")

    .def("declareProperty", (declarePropertyType2)&PythonAlgorithm::declarePyAlgProperty,
          declarePropertyType2_Overload((arg("self"), arg("name"), arg("defaultValue"), arg("validator")=object(), arg("doc")="",arg("direction")=Direction::Input),
                                        "Declares a named property where the type is taken from "
                                        "the type of the defaultValue and mapped to an appropriate C++ type"))

    .def("declareProperty", (declarePropertyType3)&PythonAlgorithm::declarePyAlgProperty,
         declarePropertyType3_Overload((arg("self"), arg("name"), arg("defaultValue"), arg("doc")="",arg("direction")=Direction::Input),
                                       "Declares a named property where the type is taken from the type "
                                       "of the defaultValue and mapped to an appropriate C++ type"))

    .def("declareProperty", (declarePropertyType4)&PythonAlgorithm::declarePyAlgProperty,
        (arg("self"), arg("name"), arg("defaultValue"), arg("direction")=Direction::Input),
         "Declares a named property where the type is taken from the type "
         "of the defaultValue and mapped to an appropriate C++ type")

    .def("getLogger", &PythonAlgorithm::getLogger, return_value_policy<reference_existing_object>(),
         "Returns a reference to this algorithm's logger")
    .def("log", &PythonAlgorithm::getLogger, return_value_policy<reference_existing_object>(),
         "Returns a reference to this algorithm's logger") // Traditional name

    // deprecated methods
    .def("setWikiSummary", &PythonAlgorithm::setWikiSummary, "(Deprecated.) Set summary for the help.")
  ;

  // Prior to version 3.2 there was a separate C++ PythonAlgorithm class that inherited from Algorithm and the "PythonAlgorithm"
  // name was a distinct class in Python from the Algorithm export. In 3.2 the need for the C++ PythonAlgorithm class
  // was removed in favour of simply adapting the Algorithm base class. A lot of client code relies on the "PythonAlgorithm" name in
  // Python so we simply add an alias of the Algorithm name to PythonAlgorithm
  scope().attr("PythonAlgorithm") = scope().attr("Algorithm");

}
