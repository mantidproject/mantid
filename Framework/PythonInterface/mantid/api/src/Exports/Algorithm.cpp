// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
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
#include "MantidPythonInterface/core/Converters/PyNativeTypeExtractor.h"
#include "MantidPythonInterface/core/GetPointer.h"

#include <boost/optional.hpp>
#include <boost/python/bases.hpp>
#include <boost/python/class.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/exception_translator.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/raw_function.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/scope.hpp>
#include <boost/variant.hpp>
#include <boost/variant/static_visitor.hpp>

#include <cstddef>
#include <string>
#include <variant>
#include <vector>

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
using declarePropertyType1 = void (*)(boost::python::object &, Mantid::Kernel::Property *, const std::string &);
// declarePyAlgProperty(name, defaultValue, validator, doc, direction)
using declarePropertyType2 = void (*)(boost::python::object &, const std::string &, const boost::python::object &,
                                      const boost::python::object &, const std::string &, const int);
// declarePyAlgProperty(name, defaultValue, doc, direction)
using declarePropertyType3 = void (*)(boost::python::object &, const std::string &, const boost::python::object &,
                                      const std::string &, const int);
// declarePyAlgProperty(name, defaultValue, direction)
using declarePropertyType4 = void (*)(boost::python::object &, const std::string &, const boost::python::object &,
                                      const int);

GNU_DIAG_OFF("unused-local-typedef")
// Ignore -Wconversion warnings coming from boost::python
// Seen with GCC 7.1.1 and Boost 1.63.0
GNU_DIAG_OFF("conversion")
// Overload types
BOOST_PYTHON_FUNCTION_OVERLOADS(declarePropertyType1_Overload, PythonAlgorithm::declarePyAlgProperty, 2, 3)
BOOST_PYTHON_FUNCTION_OVERLOADS(declarePropertyType2_Overload, PythonAlgorithm::declarePyAlgProperty, 3, 6)
BOOST_PYTHON_FUNCTION_OVERLOADS(declarePropertyType3_Overload, PythonAlgorithm::declarePyAlgProperty, 4, 5)

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

template <typename T> boost::optional<T> extractArg(ssize_t index, const tuple &args) {
  if (index < len(args)) {
    return boost::optional<T>(extract<T>(args[index]));
  }
  return boost::none;
}

template <typename T> void extractKwargs(const dict &kwargs, const std::string &keyName, boost::optional<T> &out) {
  if (kwargs.has_key(keyName)) {
    out = boost::optional<T>(extract<T>(kwargs.get(keyName)));
  }
}

class SetPropertyVisitor final : public Mantid::PythonInterface::IPyTypeVisitor {
public:
  SetPropertyVisitor(Mantid::API::Algorithm_sptr &alg, std::string const &propName)
      : m_alg(alg), m_propName(propName) {}

  void operator()(bool value) const override { setProp(value); }
  void operator()(long value) const override { setProp(static_cast<int>(value)); }
  void operator()(double value) const override { setProp(value); }
  void operator()(std::string value) const override { m_alg->setPropertyValue(m_propName, value); }
  void operator()(Mantid::API::Workspace_sptr ws) const override { m_alg->setProperty(m_propName, std::move(ws)); }

  void operator()(std::vector<bool> value) const override { setProp(value); }
  void operator()(std::vector<long> value) const override { setProp(value); }
  void operator()(std::vector<double> value) const override { setProp(value); }
  void operator()(std::vector<std::string> value) const override { setProp(value); }

  using Mantid::PythonInterface::IPyTypeVisitor::operator();

private:
  template <typename T> void setProp(const T &val) const { m_alg->setProperty(m_propName, val); }

  Mantid::API::Algorithm_sptr &m_alg;
  std::string const &m_propName;
};

// Signature createChildWithProps(self, name, startProgress, endProgress, enableLogging, version, **kwargs)
object createChildWithProps(tuple args, dict kwargs) {
  Mantid::API::Algorithm_sptr parentAlg = extract<Mantid::API::Algorithm_sptr>(args[0]);
  auto name = extractArg<std::string>(1, args);
  auto startProgress = extractArg<double>(2, args);
  auto endProgress = extractArg<double>(3, args);
  auto enableLogging = extractArg<bool>(4, args);
  auto version = extractArg<int>(5, args);

  const std::array<std::string, 5> reservedNames = {"name", "startProgress", "endProgress", "enableLogging", "version"};

  extractKwargs<std::string>(kwargs, reservedNames[0], name);
  extractKwargs<double>(kwargs, reservedNames[1], startProgress);
  extractKwargs<double>(kwargs, reservedNames[2], endProgress);
  extractKwargs<bool>(kwargs, reservedNames[3], enableLogging);
  extractKwargs<int>(kwargs, reservedNames[4], version);

  if (!name.is_initialized()) {
    throw std::invalid_argument("Please specify the algorithm name");
  }
  auto childAlg = parentAlg->createChildAlgorithm(name.value(), startProgress.value_or(-1), endProgress.value_or(-1),
                                                  enableLogging.value_or(true), version.value_or(-1));

  const list keys = kwargs.keys();
  for (int i = 0; i < len(keys); ++i) {
    const std::string propName = extract<std::string>(keys[i]);

    if (std::find(reservedNames.cbegin(), reservedNames.cend(), propName) != reservedNames.cend())
      continue;

    object curArg = kwargs[keys[i]];
    if (!curArg)
      continue;

    using Mantid::PythonInterface::PyNativeTypeExtractor;
    auto nativeObj = PyNativeTypeExtractor::convert(curArg);

    boost::apply_visitor(SetPropertyVisitor(childAlg, propName), nativeObj);
  }
  return object(childAlg);
}

} // namespace

void export_leaf_classes() {
  register_ptr_to_python<std::shared_ptr<Algorithm>>();
  register_exception_translator<Algorithm::CancelException>(&translateCancel);

  // Export Algorithm but the actual held type in Python is
  // std::shared_ptr<AlgorithmAdapter>
  // See
  // http://wiki.python.org/moin/boost.python/HowTo#ownership_of_C.2B-.2B-_object_extended_in_Python
  class_<Algorithm, bases<Mantid::API::IAlgorithm>, std::shared_ptr<PythonAlgorithm>, boost::noncopyable>(
      "Algorithm", "Base class for all algorithms")
      .def("fromString", &Algorithm::fromString, "Initialize the algorithm from a string representation")
      .staticmethod("fromString")
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4267) // Disable warning regarding conversion of size_t
                                // to int within raw_function, which we can do nothing about
#endif
      .def("createChildAlgorithm", raw_function(&createChildWithProps, std::size_t(1)),
           "Creates and intializes a named child algorithm. Output workspaces "
           "are given a dummy name.")
#ifdef _MSC_VER
#pragma warning(pop)
#endif
      .def("declareProperty", (declarePropertyType1)&PythonAlgorithm::declarePyAlgProperty,
           declarePropertyType1_Overload((arg("self"), arg("prop"), arg("doc") = "")))
      .def("enableHistoryRecordingForChild", &Algorithm::enableHistoryRecordingForChild, (arg("self"), arg("on")),
           "Turns history recording on or off for an algorithm.")
      .def("declareProperty", (declarePropertyType2)&PythonAlgorithm::declarePyAlgProperty,
           declarePropertyType2_Overload((arg("self"), arg("name"), arg("defaultValue"), arg("validator") = object(),
                                          arg("doc") = "", arg("direction") = Direction::Input),
                                         "Declares a named property where the type is taken from "
                                         "the type of the defaultValue and mapped to an appropriate C++ "
                                         "type"))
      .def("declareProperty", (declarePropertyType3)&PythonAlgorithm::declarePyAlgProperty,
           declarePropertyType3_Overload(
               (arg("self"), arg("name"), arg("defaultValue"), arg("doc") = "", arg("direction") = Direction::Input),
               "Declares a named property where the type is taken from the "
               "type "
               "of the defaultValue and mapped to an appropriate C++ type"))
      .def("declareProperty", (declarePropertyType4)&PythonAlgorithm::declarePyAlgProperty,
           (arg("self"), arg("name"), arg("defaultValue"), arg("direction") = Direction::Input),
           "Declares a named property where the type is taken from the type "
           "of the defaultValue and mapped to an appropriate C++ type")
      .def("getLogger", &PythonAlgorithm::getLogger, arg("self"), return_value_policy<reference_existing_object>(),
           "Returns a reference to this algorithm's logger")
      .def("log", &PythonAlgorithm::getLogger, arg("self"), return_value_policy<reference_existing_object>(),
           "Returns a reference to this algorithm's logger") // Traditional name

      // deprecated methods
      .def("setWikiSummary", &PythonAlgorithm::setWikiSummary, (arg("self"), arg("summary")),
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
  register_ptr_to_python<std::shared_ptr<SerialAlgorithm>>();
  register_exception_translator<SerialAlgorithm::CancelException>(&translateCancel);
  class_<SerialAlgorithm, bases<Mantid::API::Algorithm>, std::shared_ptr<PythonSerialAlgorithm>, boost::noncopyable>(
      "SerialAlgorithm", "Base class for simple serial algorithms");
  scope().attr("PythonSerialAlgorithm") = scope().attr("SerialAlgorithm");
}

void export_ParallelAlgorithm() {
  register_ptr_to_python<std::shared_ptr<ParallelAlgorithm>>();
  register_exception_translator<ParallelAlgorithm::CancelException>(&translateCancel);
  class_<ParallelAlgorithm, bases<Mantid::API::Algorithm>, std::shared_ptr<PythonParallelAlgorithm>,
         boost::noncopyable>("ParallelAlgorithm", "Base class for simple parallel algorithms");
  scope().attr("PythonParallelAlgorithm") = scope().attr("ParallelAlgorithm");
}

void export_DistributedAlgorithm() {
  register_ptr_to_python<std::shared_ptr<DistributedAlgorithm>>();
  register_exception_translator<DistributedAlgorithm::CancelException>(&translateCancel);
  class_<DistributedAlgorithm, bases<Mantid::API::Algorithm>, std::shared_ptr<PythonDistributedAlgorithm>,
         boost::noncopyable>("DistributedAlgorithm", "Base class for simple distributed algorithms");
  scope().attr("PythonDistributedAlgorithm") = scope().attr("DistributedAlgorithm");
}
