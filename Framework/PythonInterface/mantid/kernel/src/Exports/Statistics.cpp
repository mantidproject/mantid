#include "MantidKernel/Statistics.h"
#include "MantidPythonInterface/kernel/Converters/NDArrayToVector.h"
#include "MantidPythonInterface/kernel/Policies/VectorToNumpy.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/numeric.hpp>
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
  const PyArrayObject *firstArray = (const PyArrayObject *)first;
  const PyArrayObject *secondArray = (const PyArrayObject *)second;
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
Statistics getStatisticsNumpy(const numeric::array &data,
                              const bool sorted = false) {
  using Mantid::Kernel::getStatistics;
  using Mantid::Kernel::StatOptions;
  using Converters::NDArrayToVector;

  if (isFloatArray(data.ptr())) {
    unsigned int flags = StatOptions::AllStats;
    if (sorted)
      flags |= StatOptions::SortedData;
    return getStatistics(NDArrayToVector<double>(data)(), flags);
  } else {
    throw UnknownDataType();
  }
}
// Define an overload to handle the default argument
BOOST_PYTHON_FUNCTION_OVERLOADS(getStatisticsOverloads, getStatisticsNumpy, 1,
                                2)

//============================ Z score
//============================================

/**
 * The implementation for getZscoreNumpy for using numpy arrays
 * @param zscoreFunc A function pointer to the required moments function
 * @param data Numpy array of data
 */
std::vector<double> getZscoreNumpy(const numeric::array &data) {
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
std::vector<double> getZscoreNumpyDeprecated(const numeric::array &data,
                                             const bool sorted) {
  UNUSED_ARG(sorted);
  PyErr_Warn(PyExc_DeprecationWarning,
             "getZScore no longer requires the second sorted argument.");
  return getZscoreNumpy(data);
}

/**
 * Proxy for @see Mantid::Kernel::getModifiedZscore so that it can accept numpy
 * arrays,
 */
std::vector<double> getModifiedZscoreNumpy(const numeric::array &data,
                                           const bool sorted = false) {
  using Mantid::Kernel::getModifiedZscore;
  using Converters::NDArrayToVector;

  if (isFloatArray(data.ptr())) {
    return getModifiedZscore(NDArrayToVector<double>(data)(), sorted);
  } else {
    throw UnknownDataType();
  }
}

// Define an overload to handle the default argument
BOOST_PYTHON_FUNCTION_OVERLOADS(getModifiedZscoreOverloads,
                                getModifiedZscoreNumpy, 1, 2)

//============================ getMoments
//============================================

// Function pointer to real implementation of getMoments
typedef std::vector<double>(*MomentsFunction)(const std::vector<double> &indep,
                                              const std::vector<double> &depend,
                                              const int);

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
std::vector<double> getMomentsNumpyImpl(MomentsFunction momentsFunc,
                                        const numeric::array &indep,
                                        const numeric::array &depend,
                                        const int maxMoment) {
  using Converters::NDArrayToVector;

  // Both input arrays must have the same typed data
  if (typesEqual(indep.ptr(), depend.ptr())) {
    throw std::invalid_argument("Datatypes of input arrays must match.");
  }

  if (isFloatArray(indep.ptr()) && isFloatArray(indep.ptr())) {
    return momentsFunc(NDArrayToVector<double>(indep)(),
                       NDArrayToVector<double>(depend)(), maxMoment);
  } else {
    throw UnknownDataType();
  }
}

/**
 * Proxy for @see Mantid::Kernel::getMomentsAboutOrigin so that it can accept
 * numpy arrays
 */
std::vector<double> getMomentsAboutOriginNumpy(const numeric::array &indep,
                                               const numeric::array &depend,
                                               const int maxMoment = 3) {
  using Mantid::Kernel::getMomentsAboutOrigin;
  return getMomentsNumpyImpl(&getMomentsAboutOrigin, indep, depend, maxMoment);
}

// Define an overload to handle the default argument
BOOST_PYTHON_FUNCTION_OVERLOADS(getMomentsAboutOriginOverloads,
                                getMomentsAboutOriginNumpy, 2, 3)

/**
 * Proxy for @see Mantid::Kernel::getMomentsAboutMean so that it can accept
 * numpy arrays
 */
std::vector<double> getMomentsAboutMeanNumpy(const numeric::array &indep,
                                             numeric::array &depend,
                                             const int maxMoment = 3) {
  using Mantid::Kernel::getMomentsAboutMean;
  return getMomentsNumpyImpl(&getMomentsAboutMean, indep, depend, maxMoment);
}

// Define an overload to handle the default argument
BOOST_PYTHON_FUNCTION_OVERLOADS(getMomentsAboutMeanOverloads,
                                getMomentsAboutMeanNumpy, 2, 3)
///@endcond
}

// -------------------------------------- Exports start here
// --------------------------------------

void export_Statistics() {
  // typedef std::vector --> numpy array result converter
  typedef return_value_policy<Policies::VectorToNumpy> ReturnNumpyArray;

  // define a new "Statistics" scope so that everything is called as
  // Statistics.getXXX
  // this affects everything defined within the lifetime of the scope object
  scope stats =
      class_<Stats>("Stats", no_init)
          .def("getStatistics", &getStatisticsNumpy,
               getStatisticsOverloads(
                   args("data", "sorted"),
                   "Determine the statistics for an array of data"))
          .staticmethod("getStatistics")

          .def("getZscore", &getZscoreNumpy, args("data"),
               "Determine the Z score for an array of data")
          .def("getZscore", &getZscoreNumpyDeprecated, args("data", "sorted"),
               "Determine the Z score for an array of "
               "data (deprecated sorted argument)")
          .staticmethod("getZscore")

          .def("getModifiedZscore", &getModifiedZscoreNumpy,
               getModifiedZscoreOverloads(
                   args("data", "sorted"),
                   "Determine the modified Z score for an array of data"))
          .staticmethod("getModifiedZscore")

          .def("getMomentsAboutOrigin", &getMomentsAboutOriginNumpy,
               getMomentsAboutOriginOverloads(
                   args("indep", "depend", "maxMoment"),
                   "Calculate the first n-moments (inclusive) about the origin")
                   [ReturnNumpyArray()])
          .staticmethod("getMomentsAboutOrigin")

          .def("getMomentsAboutMean", &getMomentsAboutMeanNumpy,
               getMomentsAboutMeanOverloads(
                   args("indep", "depend", "maxMoment"),
                   "Calculate the first n-moments (inclusive) about the mean")
                   [ReturnNumpyArray()])
          .staticmethod("getMomentsAboutMean");

  //------------------------------ Statistics
  // values--------------------------------
  // Want this in the same scope as above so must be here
  class_<Statistics>("Statistics")
      .add_property("minimum", &Statistics::minimum,
                    "Minimum value of the data set")
      .add_property("maximum", &Statistics::maximum,
                    "Maximum value of the data set")
      .add_property("mean", &Statistics::mean,
                    "Simple mean, sum(data)/nvalues, of the data set")
      .add_property("median", &Statistics::median,
                    "Middle value of the data set")
      .add_property("standard_deviation", &Statistics::standard_deviation,
                    "Standard width of distribution");
}
