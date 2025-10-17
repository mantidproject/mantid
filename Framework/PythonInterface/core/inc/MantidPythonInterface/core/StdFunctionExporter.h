#include "MantidPythonInterface/core/ErrorHandling.h"

#include <boost/python.hpp>
#include <boost/python/converter/registry.hpp>
#include <functional>
#include <string>
#include <type_traits>

namespace {

// Wrap raw pointers as Python objects holding the C++ pointer; nullptr -> None.
template <typename T> inline boost::python::object wrap_for_python(T *p) {
  if (p)
    return boost::python::object(boost::python::ptr(p));
  return boost::python::object(); // None
}

// Pass non-pointer arguments through as regular Python objects (numbers, strings, etc.).
template <typename T>
inline typename std::enable_if<!std::is_pointer<typename std::decay<T>::type>::value, boost::python::object>::type
wrap_for_python(T &&v) {
  return boost::python::object(std::forward<T>(v));
}

} // namespace

namespace Mantid {
namespace PythonInterface {

template <typename R, typename... Args> struct std_function_from_python {
  std_function_from_python() {
    boost::python::converter::registry::push_back(&convertible, &construct,
                                                  boost::python::type_id<std::function<R(Args...)>>());
  }

  static void *convertible(PyObject *obj_ptr) { return PyCallable_Check(obj_ptr) ? obj_ptr : nullptr; }

  static void construct(PyObject *obj_ptr, boost::python::converter::rvalue_from_python_stage1_data *data) {
    using func_t = std::function<R(Args...)>;
    void *storage =
        reinterpret_cast<boost::python::converter::rvalue_from_python_storage<func_t> *>(data)->storage.bytes;

    boost::python::handle<> hnd(boost::python::borrowed(obj_ptr));
    boost::python::object cb(hnd);

    new (storage) func_t([cb](Args... args) -> R {
      PyGILState_STATE gstate = PyGILState_Ensure();
      try {
        // Wrap each argument: pointers -> boost::python::ptr, scalars -> as-is.
        boost::python::object res = cb(wrap_for_python(std::forward<Args>(args))...);
        R out = boost::python::extract<R>(res);
        PyGILState_Release(gstate);
        return out;
      } catch (boost::python::error_already_set &) {
        PyGILState_Release(gstate);
        throw PythonException();
      }
    });

    data->convertible = storage;
  }
};

} // namespace PythonInterface
} // namespace Mantid
