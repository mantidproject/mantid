// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/IFunction.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/api/PythonAlgorithm/AlgorithmAdapter.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidPythonInterface/kernel/PythonObjectInstantiator.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <mutex>

// Python frameobject. This is under the boost includes so that boost will have
// done the
// include of Python.h which it ensures is done correctly
#include <frameobject.h>

using Mantid::API::FunctionFactory;
using Mantid::API::FunctionFactoryImpl;
using Mantid::API::IFunction;
using Mantid::PythonInterface::PythonObjectInstantiator;

using namespace boost::python;

GET_POINTER_SPECIALIZATION(FunctionFactoryImpl)

namespace Mantid {
namespace PythonInterface {

/// Specialization for IFunction. Fit functions defined in
/// python need to be wrapped in FunctionWrapper without
/// asking the user to do additional actions.
/// The instantiator lets the fit function class object know
/// that an instance will be created by the FunctionFactory
/// and it needs to be a subclass of IFunction and not a
/// FunctionWrapper.
template <>
boost::shared_ptr<IFunction>
PythonObjectInstantiator<IFunction>::createInstance() const {
  using namespace boost::python;
  GlobalInterpreterLock gil;

  // The class may instantiate different objects depending on whether
  // it is being created by the function factory or not
  bool const isClassFactoryAware =
      PyObject_HasAttrString(m_classObject.ptr(), "_factory_use");

  if (isClassFactoryAware) {
    m_classObject.attr("_factory_use")();
  }
  object instance = m_classObject();
  if (isClassFactoryAware) {
    m_classObject.attr("_factory_free")();
  }
  auto instancePtr = extract<boost::shared_ptr<IFunction>>(instance)();
  auto *deleter =
      boost::get_deleter<converter::shared_ptr_deleter, IFunction>(instancePtr);
  instancePtr.reset(instancePtr.get(), GILSharedPtrDeleter(*deleter));
  return instancePtr;
}
} // namespace PythonInterface
} // namespace Mantid

namespace {
///@cond

//------------------------------------------------------------------------------------------------------
/**
 * A Python friendly version that returns the registered functions as a list.
 * @param self :: Enables it to be called as a member function on the
 * FunctionFactory class
 */
PyObject *getFunctionNames(FunctionFactoryImpl &self) {
  const std::vector<std::string> &names =
      self.getFunctionNames<Mantid::API::IFunction>();

  PyObject *registered = PyList_New(0);
  for (const auto &name : names) {
    PyObject *value = to_python_value<const std::string &>()(name);
    if (PyList_Append(registered, value))
      throw std::runtime_error("Failed to insert value into PyList");
  }

  return registered;
}

//------------------------------------------------------------------------------------------------------
/**
 * Something that makes Function Factory return to python a composite function
 * for Product function, Convolution or
 * any similar superclass of composite function.
 * @param self :: Enables it to be called as a member function on the
 * FunctionFactory class
 * @param name :: Name of the superclass of composite function,
 * e.g. "ProductFunction".
 */
Mantid::API::CompositeFunction_sptr
createCompositeFunction(FunctionFactoryImpl &self, const std::string &name) {
  auto fun = self.createFunction(name);
  auto composite =
      boost::dynamic_pointer_cast<Mantid::API::CompositeFunction>(fun);
  if (composite) {
    return composite;
  }
  std::string error_message = name + " is not a composite function.";
  throw std::invalid_argument(error_message);
}

//----- Function registration -----

/// Python algorithm registration mutex in anonymous namespace (aka static)
std::recursive_mutex FUNCTION_REGISTER_MUTEX;

/**
 * A free function to register a fit function from Python
 * @param classObject A Python class derived from IFunction
 */
void subscribe(FunctionFactoryImpl &self, PyObject *classObject) {
  std::lock_guard<std::recursive_mutex> lock(FUNCTION_REGISTER_MUTEX);
  static auto *baseClass = const_cast<PyTypeObject *>(
      converter::registered<IFunction>::converters.to_python_target_type());
  // object mantidapi(handle<>(PyImport_ImportModule("mantid.api")));
  // object ifunction = mantidapi.attr("IFunction");

  // obj should be a class deriving from IFunction
  // PyObject_IsSubclass can set the error handler if classObject
  // is not a class so this needs to be checked in two stages
  const bool isSubClass =
      (PyObject_IsSubclass(classObject,
                           reinterpret_cast<PyObject *>(baseClass)) == 1);
  if (PyErr_Occurred() || !isSubClass) {
    throw std::invalid_argument(std::string("subscribe(): Unexpected type. "
                                            "Expected a class derived from "
                                            "IFunction1D or IPeakFunction, "
                                            "found: ") +
                                classObject->ob_type->tp_name);
  }
  // Instantiator will store a reference to the class object, so increase
  // reference count with borrowed template
  auto creator = std::make_unique<PythonObjectInstantiator<IFunction>>(
      object(handle<>(borrowed(classObject))));

  // Can the function be created and initialized? It really shouldn't go in
  // to the factory if not
  auto func = creator->createInstance();
  func->initialize();

  // Takes ownership of instantiator
  self.subscribe(func->name(), std::move(creator),
                 FunctionFactoryImpl::OverwriteCurrent);
}
///@endcond
} // namespace

void export_FunctionFactory() {

  class_<FunctionFactoryImpl, boost::noncopyable>("FunctionFactoryImpl",
                                                  no_init)
      .def("getFunctionNames", &getFunctionNames, arg("self"),
           "Returns a list of the currently available functions")
      .def("createCompositeFunction", &createCompositeFunction,
           (arg("self"), arg("name")),
           "Return a pointer to the requested function")
      .def("createFunction", &FunctionFactoryImpl::createFunction,
           (arg("self"), arg("type")),
           "Return a pointer to the requested function")
      .def("createInitialized", &FunctionFactoryImpl::createInitialized,
           (arg("self"), arg("init_expr")),
           "Return a pointer to the requested function")
      .def("subscribe", &subscribe, (arg("self"), arg("object")),
           "Register a Python class derived from IFunction into the factory")
      .def("unsubscribe", &FunctionFactoryImpl::unsubscribe,
           (arg("self"), arg("class_name")), "Remove a type from the factory")
      .def("Instance", &FunctionFactory::Instance,
           return_value_policy<reference_existing_object>(),
           "Returns a reference to the FunctionFactory singleton")
      .staticmethod("Instance");
}
