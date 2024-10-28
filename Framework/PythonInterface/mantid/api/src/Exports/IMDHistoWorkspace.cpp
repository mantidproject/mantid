// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidPythonInterface/api/RegisterWorkspacePtrToPython.h"
#include "MantidPythonInterface/core/Converters/NDArrayTypeIndex.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include "MantidPythonInterface/core/NDArray.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_non_const_reference.hpp>
#define PY_ARRAY_UNIQUE_SYMBOL API_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

using namespace Mantid::API;
using Mantid::PythonInterface::NDArray;
using Mantid::PythonInterface::Registry::RegisterWorkspacePtrToPython;
namespace Converters = Mantid::PythonInterface::Converters;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(IMDHistoWorkspace)

namespace Mantid::PythonInterface::Converters {
extern template int NDArrayTypeIndex<float>::typenum;
extern template int NDArrayTypeIndex<double>::typenum;
} // namespace Mantid::PythonInterface::Converters

namespace {
/**

 * Determine the sizes of each dimensions
 * @param array :: the C++ array
 * @param dims :: the dimensions vector (Py_intptr_t type)
 * @returns A python object containing the numpy array
 */
PyObject *WrapReadOnlyNumpyFArray(const Mantid::signal_t *arr, std::vector<Py_intptr_t> dims) {
  int datatype = Converters::NDArrayTypeIndex<Mantid::signal_t>::typenum;
#if NPY_API_VERSION >= 0x00000007 //(1.7)
  auto *nparray = reinterpret_cast<PyArrayObject *>(
      PyArray_New(&PyArray_Type, static_cast<int>(dims.size()), &dims[0], datatype, nullptr,
                  static_cast<void *>(const_cast<double *>(arr)), 0, NPY_ARRAY_FARRAY, nullptr));
  PyArray_CLEARFLAGS(nparray, NPY_ARRAY_WRITEABLE);
#else
  PyArrayObject *nparray =
      (PyArrayObject *)PyArray_New(&PyArray_Type, static_cast<int>(dims.size()), &dims[0], datatype, nullptr,
                                   static_cast<void *>(const_cast<double *>(arr)), 0, NPY_FARRAY, nullptr);
  nparray->flags &= ~NPY_WRITEABLE;
#endif
  return reinterpret_cast<PyObject *>(nparray);
}

/**
 * Determine the sizes of each dimensions
 * @param self :: A reference to the calling object
 * @returns A vector of the dimension sizes
 */
std::vector<Py_intptr_t> countDimensions(const IMDHistoWorkspace &self) {
  size_t ndims = self.getNumDims();
  std::vector<size_t> nd;
  nd.reserve(ndims);

  // invert dimensions in C way, e.g. slowest changing ndim goes first
  for (size_t i = 0; i < ndims; ++i) {
    nd.emplace_back(self.getDimension(i)->getNBins());
  }

  ndims = nd.size();
  std::vector<Py_intptr_t> dims(ndims);
  for (size_t i = 0; i < ndims; ++i)
    dims[i] = static_cast<Py_intptr_t>(nd[i]);

  if (dims.empty()) {
    throw std::runtime_error("Workspace has zero dimensions!");
  } else {
    return dims;
  }
}

/**
 * Returns the signal array from the workspace as a numpy array
 * @param self :: A reference to the calling object
 */
PyObject *getSignalArrayAsNumpyArray(const IMDHistoWorkspace &self) {
  auto dims = countDimensions(self);
  return WrapReadOnlyNumpyFArray(self.getSignalArray(), dims);
}

/**
 * Returns the error squared array from the workspace as a numpy array
 * @param self :: A reference to the calling object
 */
PyObject *getErrorSquaredArrayAsNumpyArray(const IMDHistoWorkspace &self) {
  auto dims = countDimensions(self);
  return WrapReadOnlyNumpyFArray(self.getErrorSquaredArray(), dims);
}

/**
 * Returns the number of events array from the workspace as a numpy array
 * @param self :: A reference to the calling object
 */
PyObject *getNumEventsArrayAsNumpyArray(const IMDHistoWorkspace &self) {
  auto dims = countDimensions(self);
  return WrapReadOnlyNumpyFArray(self.getNumEventsArray(), dims);
}

/**
 * Checks the size of the given array against the given MDHistoWorkspace to see
 * if they match. Throws if not
 * @param self :: The calling object
 * @param signal :: The new values
 * @param fnLabel :: A message prefix to pass if the sizes are incorrect
 */
void throwIfSizeIncorrect(const IMDHistoWorkspace &self, const NDArray &signal, const std::string &fnLabel) {
  auto wsShape = countDimensions(self);
  const size_t ndims = wsShape.size();
  auto arrShape = signal.attr("shape");
  if (ndims != static_cast<size_t>(len(arrShape))) {
    std::ostringstream os;
    os << fnLabel
       << ": The number of  dimensions doe not match the current "
          "workspace size. Workspace="
       << ndims << " array=" << len(arrShape);
    throw std::invalid_argument(os.str());
  }

  for (size_t i = 0; i < ndims; ++i) {
    int arrDim = extract<int>(arrShape[i])();
    if (wsShape[i] != arrDim) {
      std::ostringstream os;
      os << fnLabel << ": The dimension size for the " << std::to_string(i) << "th dimension do not match. "
         << "Workspace dimension size=" << wsShape[i] << ", array size=" << arrDim;
      throw std::invalid_argument(os.str());
    }
  }
}

/**
 * Set the signal array from a numpy array. This simply loops over the array &
 * sets each value
 * It does not allow the workspace dimensions to be resized, it will throw if
 * the sizes are not
 * correct
 */
void setSignalArray(IMDHistoWorkspace &self, const NDArray &signalValues) {
  throwIfSizeIncorrect(self, signalValues, "setSignalArray");
  object rav = signalValues.attr("ravel")("F");
  object flattened = rav.attr("flat");
  auto length = len(flattened);
  for (auto i = 0; i < length; ++i) {
    self.setSignalAt(i, extract<double>(flattened[i])());
  }
}

/**
 * Set the square of the errors array from a numpy array. This simply loops over
 * the array & sets each value
 * It does not allow the workspace dimensions to be resized, it will throw if
 * the sizes are not
 * correct
 */
void setErrorSquaredArray(IMDHistoWorkspace &self, const NDArray &errorSquared) {
  throwIfSizeIncorrect(self, errorSquared, "setErrorSquaredArray");
  object rav = errorSquared.attr("ravel")("F");
  object flattened = rav.attr("flat");
  auto length = len(flattened);
  for (auto i = 0; i < length; ++i) {
    self.setErrorSquaredAt(i, extract<double>(flattened[i])());
  }
}

/**
 * Set the signal at a specific index in the workspace
 */
void setSignalAt(IMDHistoWorkspace &self, const size_t index, const double value) {
  if (index >= self.getNPoints())
    throw std::invalid_argument("setSignalAt: The index is greater than the "
                                "number of bins in the workspace");

  self.setSignalAt(index, value);
}
} // namespace

void export_IMDHistoWorkspace() {
  // IMDHistoWorkspace class
  class_<IMDHistoWorkspace, bases<IMDWorkspace, MultipleExperimentInfos>, boost::noncopyable>("IMDHistoWorkspace",
                                                                                              no_init)
      .def("getSignalArray", &getSignalArrayAsNumpyArray, arg("self"),
           "Returns a read-only numpy array containing the signal values")

      .def("getErrorSquaredArray", &getErrorSquaredArrayAsNumpyArray, arg("self"),
           "Returns a read-only numpy array containing the square of the error "
           "values")

      .def("getNumEventsArray", &getNumEventsArrayAsNumpyArray, arg("self"),
           "Returns a read-only numpy array containing the number of MD events "
           "in each bin")

      .def("signalAt", &IMDHistoWorkspace::signalAt, (arg("self"), arg("index")),
           return_value_policy<copy_non_const_reference>(), "Return a reference to the signal at the linear index")

      .def("errorSquaredAt", &IMDHistoWorkspace::errorSquaredAt, (arg("self"), arg("index")),
           return_value_policy<copy_non_const_reference>(), "Return the squared-errors at the linear index")

      .def("setSignalAt", &setSignalAt, (arg("self"), arg("index"), arg("value")),
           "Sets the signal at the specified index.")

      .def("setErrorSquaredAt", &IMDHistoWorkspace::setErrorSquaredAt, (arg("self"), arg("index"), arg("value")),
           "Sets the squared-error at the specified index.")

      .def("setSignalArray", &setSignalArray, (arg("self"), arg("signalValues")),
           "Sets the signal from a numpy array. The sizes must match the "
           "current workspace sizes. A ValueError is thrown if not")

      .def("setErrorSquaredArray", &setErrorSquaredArray, (arg("self"), arg("errorSquared")),
           "Sets the square of the errors from a numpy array. The sizes must "
           "match the current workspace sizes. A ValueError is thrown if not")

      .def("setTo", &IMDHistoWorkspace::setTo, (arg("self"), arg("signal"), arg("error_squared"), arg("num_events")),
           "Sets all signals/errors in the workspace to the given values")

      .def("getInverseVolume", &IMDHistoWorkspace::getInverseVolume, arg("self"),
           return_value_policy<return_by_value>(), "Return the inverse of volume of EACH cell in the workspace.")

      .def("getLinearIndex", (size_t(IMDHistoWorkspace::*)(size_t, size_t) const)&IMDHistoWorkspace::getLinearIndex,
           (arg("self"), arg("index1"), arg("index2")), return_value_policy<return_by_value>(),
           "Get the 1D linear index from the 2D array")

      .def("getLinearIndex",
           (size_t(IMDHistoWorkspace::*)(size_t, size_t, size_t) const)&IMDHistoWorkspace::getLinearIndex,
           (arg("self"), arg("index1"), arg("index2"), arg("index3")), return_value_policy<return_by_value>(),
           "Get the 1D linear index from the 3D array")

      .def("getLinearIndex",
           (size_t(IMDHistoWorkspace::*)(size_t, size_t, size_t, size_t) const)&IMDHistoWorkspace::getLinearIndex,
           (arg("self"), arg("index1"), arg("index2"), arg("index3"), arg("index4")),
           return_value_policy<return_by_value>(), "Get the 1D linear index from the 4D array")

      .def("getCenter", &IMDHistoWorkspace::getCenter, (arg("self"), arg("linear_index")),
           return_value_policy<return_by_value>(), "Return the position of the center of a bin at a given position")

      .def("setDisplayNormalization", &IMDHistoWorkspace::setDisplayNormalization, (arg("self"), arg("normalization")),
           "Sets the visual normalization of"
           " the workspace.");

  //-------------------------------------------------------------------------------------------------

  RegisterWorkspacePtrToPython<IMDHistoWorkspace>();
}
