// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifdef _MSC_VER
#pragma warning(disable : 4250) // Disable warning regarding inheritance via
                                // dominance, we have no way around it with the
                                // design
#endif
#include "MantidAPI/DistributedAlgorithm.h"
#include "MantidAPI/ParallelAlgorithm.h"
#include "MantidAPI/SerialAlgorithm.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/api/PythonAlgorithm/AlgorithmAdapter.h"
#ifdef _MSC_VER
#pragma warning(default : 4250)
#endif
#include "MantidPythonInterface/core/GetPointer.h"

#include <boost/python/bases.hpp>
#include <boost/python/class.hpp>
#include <boost/python/exception_translator.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/scope.hpp>

using Mantid::API::Algorithm;
using Mantid::API::DistributedAlgorithm;
using Mantid::API::ParallelAlgorithm;
using Mantid::API::SerialAlgorithm;
using Mantid::Kernel::Direction;
using Mantid::PythonInterface::AlgorithmAdapter;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(Algorithm)
GET_POINTER_SPECIALIZATION(SerialAlgorithm)
GET_POINTER_SPECIALIZATION(ParallelAlgorithm)
GET_POINTER_SPECIALIZATION(DistributedAlgorithm)

namespace {
using PythonAlgorithm = AlgorithmAdapter<Algorithm>;
using PythonSerialAlgorithm = AlgorithmAdapter<SerialAlgorithm>;
using PythonParallelAlgorithm = AlgorithmAdapter<ParallelAlgorithm>;
using PythonDistributedAlgorithm = AlgorithmAdapter<DistributedAlgorithm>;

// declarePyAlgProperty(property*,doc)
using declarePropertyType1 = void (*)(boost::python::object &,
                                      Mantid::Kernel::Property *,
                                      const std::string &);
// declarePyAlgProperty(name, defaultValue, validator, doc, direction)
using declarePropertyType2 = void (*)(boost::python::object &,
                                      const std::string &,
                                      const boost::python::object &,
                                      const boost::python::object &,
                                      const std::string &, const int);
// declarePyAlgProperty(name, defaultValue, doc, direction)
using declarePropertyType3 = void (*)(boost::python::object &,
                                      const std::string &,
                                      const boost::python::object &,
                                      const std::string &, const int);
// declarePyAlgProperty(name, defaultValue, direction)
using declarePropertyType4 = void (*)(boost::python::object &,
                                      const std::string &,
                                      const boost::python::object &, const int);
GNU_DIAG_OFF("unused-local-typedef")
// Ignore -Wconversion warnings coming from boost::python
// Seen with GCC 7.1.1 and Boost 1.63.0
GNU_DIAG_OFF("conversion")
// Overload types
BOOST_PYTHON_FUNCTION_OVERLOADS(declarePropertyType1_Overload,
                                PythonAlgorithm::declarePyAlgProperty, 2, 3)
BOOST_PYTHON_FUNCTION_OVERLOADS(declarePropertyType2_Overload,
                                PythonAlgorithm::declarePyAlgProperty, 3, 6)
BOOST_PYTHON_FUNCTION_OVERLOADS(declarePropertyType3_Overload,
                                PythonAlgorithm::declarePyAlgProperty, 4, 5)
GNU_DIAG_ON("conversion")
GNU_DIAG_ON("unused-local-typedef")

/**
 * Map a CancelException to a Python KeyboardInterupt
 * @param exc A cancel exception to translate. Unused here as the message is
 * ignored
 */
void translateCancel(const Algorithm::CancelException &exc) {
  UNUSED_ARG(exc);
  PyErr_SetString(PyExc_KeyboardInterrupt, "");
}
} // namespace

void export_leaf_classes() {
  register_ptr_to_python<boost::shared_ptr<Algorithm>>();
  register_exception_translator<Algorithm::CancelException>(&translateCancel);

  // Export Algorithm but the actual held type in Python is
  // boost::shared_ptr<AlgorithmAdapter>
  // See
  // http://wiki.python.org/moin/boost.python/HowTo#ownership_of_C.2B-.2B-_object_extended_in_Python

  class_<Algorithm, bases<Mantid::API::IAlgorithm>,
         boost::shared_ptr<PythonAlgorithm>, boost::noncopyable>(
      "Algorithm", "Base class for all algorithms")
      .def("fromString", &Algorithm::fromString,
           "Initialize the algorithm from a string representation")
      .staticmethod("fromString")
      .def("createChildAlgorithm", &Algorithm::createChildAlgorithm,
           (arg("self"), arg("name"), arg("startProgress") = -1.0,
            arg("endProgress") = -1.0, arg("enableLogging") = true,
            arg("version") = -1),
           "Creates and intializes a named child algorithm. Output workspaces "
           "are given a dummy name.")
      .def("declareProperty",
           (declarePropertyType1)&PythonAlgorithm::declarePyAlgProperty,
           declarePropertyType1_Overload(
               (arg("self"), arg("prop"), arg("doc") = "")))
      .def("enableHistoryRecordingForChild",
           &Algorithm::enableHistoryRecordingForChild, (arg("self"), arg("on")),
           "Turns history recording on or off for an algorithm.")
      .def("declareProperty",
           (declarePropertyType2)&PythonAlgorithm::declarePyAlgProperty,
           declarePropertyType2_Overload(
               (arg("self"), arg("name"), arg("defaultValue"),
                arg("validator") = object(), arg("doc") = "",
                arg("direction") = Direction::Input),
               "Declares a named property where the type is taken from "
               "the type of the defaultValue and mapped to an appropriate C++ "
               "type"))
      .def("declareProperty",
           (declarePropertyType3)&PythonAlgorithm::declarePyAlgProperty,
           declarePropertyType3_Overload(
               (arg("self"), arg("name"), arg("defaultValue"), arg("doc") = "",
                arg("direction") = Direction::Input),
               "Declares a named property where the type is taken from the "
               "type "
               "of the defaultValue and mapped to an appropriate C++ type"))
      .def("declareProperty",
           (declarePropertyType4)&PythonAlgorithm::declarePyAlgProperty,
           (arg("self"), arg("name"), arg("defaultValue"),
            arg("direction") = Direction::Input),
           "Declares a named property where the type is taken from the type "
           "of the defaultValue and mapped to an appropriate C++ type")
      .def("getLogger", &PythonAlgorithm::getLogger, arg("self"),
           return_value_policy<reference_existing_object>(),
           "Returns a reference to this algorithm's logger")
      .def("log", &PythonAlgorithm::getLogger, arg("self"),
           return_value_policy<reference_existing_object>(),
           "Returns a reference to this algorithm's logger") // Traditional name

      // deprecated methods
      .def("setWikiSummary", &PythonAlgorithm::setWikiSummary,
           (arg("self"), arg("summary")),
           "(Deprecated.) Set summary for the help.");

  // Prior to version 3.2 there was a separate C++ PythonAlgorithm class that
  // inherited from Algorithm and the "PythonAlgorithm"
  // name was a distinct class in Python from the Algorithm export. In 3.2 the
  // need for the C++ PythonAlgorithm class
  // was removed in favour of simply adapting the Algorithm base class. A lot of
  // client code relies on the "PythonAlgorithm" name in
  // Python so we simply add an alias of the Algorithm name to PythonAlgorithm
  scope().attr("PythonAlgorithm") = scope().attr("Algorithm");
}

void export_SerialAlgorithm() {
  register_ptr_to_python<boost::shared_ptr<SerialAlgorithm>>();
  register_exception_translator<SerialAlgorithm::CancelException>(
      &translateCancel);
  class_<SerialAlgorithm, bases<Mantid::API::Algorithm>,
         boost::shared_ptr<PythonSerialAlgorithm>, boost::noncopyable>(
      "SerialAlgorithm", "Base class for simple serial algorithms");
  scope().attr("PythonSerialAlgorithm") = scope().attr("SerialAlgorithm");
}

void export_ParallelAlgorithm() {
  register_ptr_to_python<boost::shared_ptr<ParallelAlgorithm>>();
  register_exception_translator<ParallelAlgorithm::CancelException>(
      &translateCancel);
  class_<ParallelAlgorithm, bases<Mantid::API::Algorithm>,
         boost::shared_ptr<PythonParallelAlgorithm>, boost::noncopyable>(
      "ParallelAlgorithm", "Base class for simple parallel algorithms");
  scope().attr("PythonParallelAlgorithm") = scope().attr("ParallelAlgorithm");
}

void export_DistributedAlgorithm() {
  register_ptr_to_python<boost::shared_ptr<DistributedAlgorithm>>();
  register_exception_translator<DistributedAlgorithm::CancelException>(
      &translateCancel);
  class_<DistributedAlgorithm, bases<Mantid::API::Algorithm>,
         boost::shared_ptr<PythonDistributedAlgorithm>, boost::noncopyable>(
      "DistributedAlgorithm", "Base class for simple distributed algorithms");
  scope().attr("PythonDistributedAlgorithm") =
      scope().attr("DistributedAlgorithm");
}
