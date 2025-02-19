// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/Statistics.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/core/Converters/NDArrayToVector.h"
#include "MantidPythonInterface/core/NDArray.h"
#include "MantidPythonInterface/core/Policies/VectorToNumpy.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/scope.hpp>

// See
// http://docs.scipy.org/doc/numpy/reference/c-api.array.html#PY_ARRAY_UNIQUE_SYMBOL
#define PY_ARRAY_UNIQUE_SYMBOL KERNEL_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

using Mantid::Kernel::Statistics;
using namespace Mantid::PythonInterface;
using namespace boost::python;

namespace {
///@cond

// Dummy class used to define Stats "namespace" in python
class Stats {};

// For all methods below we have to extract specific types from Python to C++.
// We choose to support only Python float arrays (C++ double)

/**
 * Return true if the array contains float data. Equivalent to
 * PyArray_ISFLOAT but uses correct constness for numpy >= 1.7
 * @param obj A pointer to a numpy array as a plain Python object
 * @return True if the array contains float data
 */
bool isFloatArray(PyObject *obj) {
#if NPY_API_VERSION >= 0x00000007 // 1.7
  return PyArray_ISFLOAT((const PyArrayObject *)obj);
#else
  return PyArray_ISFLOAT((PyArrayObject *)obj);
#endif
}

/**
 * Return true if the two arrays contains the same type. Equivalent
 * to (PyARRAY_TYPE == PyArray_TYPE) but ses correct constness
 * for numpy >= 1.7
 * @param first A pointer to a numpy array as a plain Python object
 * @param second A pointer to a numpy array as a plain Python object
 * @return True if the array contains float data
 */
bool typesEqual(PyObject *first, PyObject *second) {
#if NPY_API_VERSION >= 0x00000007 // 1.7
  const auto *firstArray = reinterpret_cast<const PyArrayObject *>(first);
  const auto *secondArray = reinterpret_cast<const PyArrayObject *>(second);
#else
  PyArrayObject *firstArray = (PyArrayObject *)first;
  PyArrayObject *secondArray = (PyArrayObject *)second;
#endif
  return PyArray_TYPE(firstArray) != PyArray_TYPE(secondArray);
}

/// Custom exception type for unknown data type
class UnknownDataType : public std::invalid_argument {
public:
  UnknownDataType()
      : std::invalid_argument("Unknown datatype. Currently only arrays of "
                              "Python floats are supported ") {}
};

//============================ getStatistics
//============================================

/**
 * Proxy for @see Mantid::Kernel::getStatistics so that it can accept numpy
 * arrays
 * @param data Input data
 * @param sorted A boolean indicating whether the data is sorted
 */
Statistics getStatisticsNumpy(const NDArray &data, const bool sorted = false) {
  using Converters::NDArrayToVector;
  using Mantid::Kernel::getStatistics;
  using Mantid::Kernel::StatOptions;

  if (isFloatArray(data.ptr())) {
    unsigned int flags = StatOptions::AllStats;
    if (sorted)
      flags |= StatOptions::SortedData;
    return getStatistics(NDArrayToVector<double>(data)(), flags);
  } else {
    throw UnknownDataType();
  }
}

GNU_DIAG_OFF("unused-local-typedef")
// Ignore -Wconversion warnings coming from boost::python
// Seen with GCC 7.1.1 and Boost 1.63.0
GNU_DIAG_OFF("conversion")

// Define an overload to handle the default argument
// cppcheck-suppress unknownMacro
BOOST_PYTHON_FUNCTION_OVERLOADS(getStatisticsOverloads, getStatisticsNumpy, 1, 2)
GNU_DIAG_ON("conversion")
GNU_DIAG_ON("unused-local-typedef")
//============================ Z score
//============================================

/**
 * The implementation for getZscoreNumpy for using numpy arrays
 * @param zscoreFunc A function pointer to the required moments function
 * @param data Numpy array of data
 */
std::vector<double> getZscoreNumpy(const NDArray &data) {
  using Converters::NDArrayToVector;
  using Mantid::Kernel::getZscore;

  if (isFloatArray(data.ptr())) {
    return getZscore(NDArrayToVector<double>(data)());
  } else {
    throw UnknownDataType();
  }
}

/**
 * Proxy for @see Mantid::Kernel::getZscore so that it can accept numpy arrays,
 * @param data The input array
 * @param sorted True if the data is sorted (deprecated)
 */
std::vector<double> getZscoreNumpyDeprecated(const NDArray &data, const bool sorted) {
  UNUSED_ARG(sorted);
  PyErr_Warn(PyExc_DeprecationWarning, "getZScore no longer requires the second sorted argument.");
  return getZscoreNumpy(data);
}

/**
 * Proxy for @see Mantid::Kernel::getModifiedZscore so that it can accept numpy
 * arrays,
 */
std::vector<double> getModifiedZscoreNumpy(const NDArray &data, const bool sorted = false) {
  UNUSED_ARG(sorted) // We explicitly check in the kernel now
  using Converters::NDArrayToVector;
  using Mantid::Kernel::getModifiedZscore;

  if (isFloatArray(data.ptr())) {
    return getModifiedZscore(NDArrayToVector<double>(data)());
  } else {
    throw UnknownDataType();
  }
}

GNU_DIAG_OFF("unused-local-typedef")
// Ignore -Wconversion warnings coming from boost::python
// Seen with GCC 7.1.1 and Boost 1.63.0
GNU_DIAG_OFF("conversion")

// Define an overload to handle the default argument
BOOST_PYTHON_FUNCTION_OVERLOADS(getModifiedZscoreOverloads, getModifiedZscoreNumpy, 1, 2)
GNU_DIAG_ON("conversion")
GNU_DIAG_ON("unused-local-typedef")

//============================ getMoments
//============================================

// Function pointer to real implementation of getMoments
using MomentsFunction = std::vector<double> (*)(const std::vector<double> &, const std::vector<double> &, const int);

/**
 * The implementation for getMomentsAboutOrigin & getMomentsAboutOriginMean for
 * using
 * numpy arrays are identical. This encapsulates that behaviour an additional
 * parameter for
 * specifying the actual function called along.
 * @param momentsFunc A function pointer to the required moments function
 * @param indep Numpy array of independent variables
 * @param depend Numpy array of dependent variables
 * @param maxMoment Maximum number of moments to return
 */
std::vector<double> getMomentsNumpyImpl(MomentsFunction momentsFunc, const NDArray &indep, const NDArray &depend,
                                        const int maxMoment) {
  using Converters::NDArrayToVector;

  // Both input arrays must have the same typed data
  if (typesEqual(indep.ptr(), depend.ptr())) {
    throw std::invalid_argument("Datatypes of input arrays must match.");
  }

  if (isFloatArray(indep.ptr()) && isFloatArray(indep.ptr())) {
    return momentsFunc(NDArrayToVector<double>(indep)(), NDArrayToVector<double>(depend)(), maxMoment);
  } else {
    throw UnknownDataType();
  }
}

/**
 * Proxy for @see Mantid::Kernel::getMomentsAboutOrigin so that it can accept
 * numpy arrays
 */
std::vector<double> getMomentsAboutOriginNumpy(const NDArray &indep, const NDArray &depend, const int maxMoment = 3) {
  using Mantid::Kernel::getMomentsAboutOrigin;
  return getMomentsNumpyImpl(&getMomentsAboutOrigin, indep, depend, maxMoment);
}

GNU_DIAG_OFF("unused-local-typedef")
// Ignore -Wconversion warnings coming from boost::python
// Seen with GCC 7.1.1 and Boost 1.63.0
GNU_DIAG_OFF("conversion")
// Define an overload to handle the default argument
BOOST_PYTHON_FUNCTION_OVERLOADS(getMomentsAboutOriginOverloads, getMomentsAboutOriginNumpy, 2, 3)
GNU_DIAG_ON("conversion")
GNU_DIAG_ON("unused-local-typedef")
/**
 * Proxy for @see Mantid::Kernel::getMomentsAboutMean so that it can accept
 * numpy arrays
 */
std::vector<double> getMomentsAboutMeanNumpy(const NDArray &indep, NDArray &depend, const int maxMoment = 3) {
  using Mantid::Kernel::getMomentsAboutMean;
  return getMomentsNumpyImpl(&getMomentsAboutMean, indep, depend, maxMoment);
}

GNU_DIAG_OFF("unused-local-typedef")
// Ignore -Wconversion warnings coming from boost::python
// Seen with GCC 7.1.1 and Boost 1.63.0
GNU_DIAG_OFF("conversion")
// Define an overload to handle the default argument
BOOST_PYTHON_FUNCTION_OVERLOADS(getMomentsAboutMeanOverloads, getMomentsAboutMeanNumpy, 2, 3)
GNU_DIAG_ON("conversion")
GNU_DIAG_ON("unused-local-typedef")

///@endcond
} // namespace

// -------------------------------------- Exports start here
// --------------------------------------

void export_Statistics() {
  // typedef std::vector --> numpy array result converter
  using ReturnNumpyArray = return_value_policy<Policies::VectorToNumpy>;

  // define a new "Statistics" scope so that everything is called as
  // Statistics.getXXX
  // this affects everything defined within the lifetime of the scope object
  scope stats =
      class_<Stats>("Stats", no_init)
          .def("getStatistics", &getStatisticsNumpy,
               getStatisticsOverloads((arg("data"), arg("sorted")), "Determine the statistics for an array of data"))
          .staticmethod("getStatistics")

          .def("getZscore", &getZscoreNumpy, arg("data"), "Determine the Z score for an array of data")
          .def("getZscore", &getZscoreNumpyDeprecated, (arg("data"), arg("sorted")),
               "Determine the Z score for an array of "
               "data (deprecated + ignored sorted argument)")
          .staticmethod("getZscore")

          .def("getModifiedZscore", &getModifiedZscoreNumpy,
               getModifiedZscoreOverloads((arg("data"), arg("sorted")),
                                          "Determine the modified Z score for an array of data"))
          .staticmethod("getModifiedZscore")

          .def("getMomentsAboutOrigin", &getMomentsAboutOriginNumpy,
               getMomentsAboutOriginOverloads(
                   (arg("indep"), arg("depend"), arg("maxMoment")),
                   "Calculate the first n-moments (inclusive) about the origin")[ReturnNumpyArray()])
          .staticmethod("getMomentsAboutOrigin")

          .def("getMomentsAboutMean", &getMomentsAboutMeanNumpy,
               getMomentsAboutMeanOverloads(
                   (arg("indep"), arg("depend"), arg("maxMoment")),
                   "Calculate the first n-moments (inclusive) about the mean")[ReturnNumpyArray()])
          .staticmethod("getMomentsAboutMean");

  // Want this in the same scope as above so must be here
  class_<Statistics>("Statistics")
      .add_property("minimum", &Statistics::minimum, "Minimum value of the data set")
      .add_property("maximum", &Statistics::maximum, "Maximum value of the data set")
      .add_property("mean", &Statistics::mean, "Simple mean, sum(data)/nvalues, of the data set")
      .add_property("median", &Statistics::median, "Middle value of the data set")
      .add_property("standard_deviation", &Statistics::standard_deviation, "Standard width of distribution");
}
