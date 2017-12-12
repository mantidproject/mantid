#ifndef PYTHONINTERFACE_KERNEL_NDARRAY_H_
#define PYTHONINTERFACE_KERNEL_NDARRAY_H_
#include "MantidPythonInterface/kernel/DllConfig.h"

#include <boost/python/object.hpp>

namespace Mantid {
namespace PythonInterface {
namespace NumPy {

/**
 * Thin object wrapper around a numpy array. This is intended to take the place
 * of boost::python::numeric::array, which is a dated wrapper containing a bug
 * when used with Python 3 - https://github.com/boostorg/python/issues/75.
 *
 * Only minimal functionality has been ported here.
 */
class PYTHON_KERNEL_DLL NdArray : public boost::python::object {
public:
  static bool check(const boost::python::object &obj);

  NdArray(const boost::python::object &obj);
  BOOST_PYTHON_FORWARD_OBJECT_CONSTRUCTORS(NdArray, boost::python::object);

  Py_intptr_t const *get_shape() const;
  int get_nd() const;
  void *get_data() const;

  NdArray astype(char dtype, bool copy = true) const;
};

} // end namespace NumPy
} // end namespace PythonInterface
} // end namespace Mantid

namespace boost {
namespace python {
namespace converter {
/**
 * Register ndarray as a type that manages a PyObject* internally.
 */
template <>
struct PYTHON_KERNEL_DLL
    object_manager_traits<Mantid::PythonInterface::NumPy::NdArray> {
  BOOST_STATIC_CONSTANT(bool, is_specialized = true);
  static bool check(PyObject *obj);
  static python::detail::new_reference adopt(PyObject *obj);
  static PyTypeObject const *get_pytype();
};
} // end namespace converter
} // end namespace python
} // end namespace boost

#endif // PYTHONINTERFACE_KERNEL_NDARRAY_H_
