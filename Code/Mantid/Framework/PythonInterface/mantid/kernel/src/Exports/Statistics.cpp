#include "MantidKernel/Statistics.h"
#include "MantidPythonInterface/kernel/Converters/NDArrayToVector.h"
#include "MantidPythonInterface/kernel/Policies/VectorToNumpy.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/numeric.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/scope.hpp>

// See http://docs.scipy.org/doc/numpy/reference/c-api.array.html#PY_ARRAY_UNIQUE_SYMBOL
#define PY_ARRAY_UNIQUE_SYMBOL KERNEL_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

using Mantid::Kernel::Statistics;
using namespace Mantid::PythonInterface;
using namespace boost::python;

namespace
{
  ///@cond

  // Dummy class used to define Stats "namespace" in python
  class Stats {};

  // For all methods below we have to extract specific types from Python to C++.
  // We choose to support only a subset of the C++ exported types mainly:
  //   * C++ double --> Python float
  //   * C++ long --> Python int

  /// Custom exception type for unknown data type
  class UnknownDataType : public std::invalid_argument
  {
  public:
    UnknownDataType(const std::string & methodName)
      : std::invalid_argument(methodName + "(): Unknown datatype. Currently only arrays of "
                              "Python ints or floats are supported ")
    {}
  };

  /**
   * Proxy for @see Mantid::Kernel::getStatistics so that it can accept numpy arrays,
   */
  Statistics getStatisticsNumpy(const numeric::array & data, const bool sorted = false)
  {
    using Mantid::Kernel::getStatistics;
    using Converters::NDArrayToVector;

    auto *dataPtr = data.ptr();
    if(PyArray_ISFLOAT(dataPtr))
    {
      return getStatistics(NDArrayToVector<double>(data)(), sorted);
    }
    else if(PyArray_ISINTEGER(dataPtr))
    {
      return getStatistics(NDArrayToVector<long>(data)(), sorted);
    }
    else
    {
      throw UnknownDataType("getStatistics");
    }
  }
  // Define an overload to handle the default argument
  BOOST_PYTHON_FUNCTION_OVERLOADS(getStatisticsOverloads, getStatisticsNumpy, 1, 2);


  /**
   * Proxy for @see Mantid::Kernel::getMomentsAboutOrigin so that it can accept numpy arrays
   */
  std::vector<double> getMomentsAboutOriginNumpy(const numeric::array& indep, numeric::array& depend,
                                                 const int maxMoment = 3)
  {
    using Mantid::Kernel::getMomentsAboutOrigin;
    using Converters::NDArrayToVector;

    auto *indepPtr = indep.ptr();
    auto *dependPtr = depend.ptr();
    // Both input arrays must have the same typed data
    if(PyArray_TYPE((PyArrayObject*)indepPtr) != PyArray_TYPE((PyArrayObject*)dependPtr))
    {
      throw std::invalid_argument("getMomentsAboutOrigin() : Datatypes of input arrays must match.");
    }

    if(PyArray_ISFLOAT(indepPtr) && PyArray_ISFLOAT(dependPtr))
    {
      return getMomentsAboutOrigin(NDArrayToVector<double>(indep)(),
                                   NDArrayToVector<double>(depend)(), maxMoment);
    }
    else
    {
      throw UnknownDataType("getMomentsAboutOrigin");
    }
  }

  // Define an overload to handle the default argument
  BOOST_PYTHON_FUNCTION_OVERLOADS(getMomentsAboutOriginOverloads, getMomentsAboutOriginNumpy, 2, 3);
  ///@endcond

}

void export_Statistics()
{
  // typedef std::vector --> numpy array result converter
  typedef return_value_policy<Policies::VectorToNumpy> ReturnNumpyArray;

 // define a new "Statistics" scope so that everything is called as Statistics.getXXX
  // this affects everything defined within the lifetime of the scope object
  scope stats = class_<Stats>("Stats",no_init)
      .def("getStatistics", &getStatisticsNumpy,
           getStatisticsOverloads(args("data", "sorted"),
                                  "Determine the statistics for an array of data")
                                 )
     .staticmethod("getStatistics")

     .def("getMomentsAboutOrigin", &getMomentsAboutOriginNumpy,
          getMomentsAboutOriginOverloads(args("indep", "depend", "maxMoment"),
                                         "Calculate the first n-moments (inclusive) about the origin"
                                        )[ReturnNumpyArray()])
     .staticmethod("getMomentsAboutOrigin")

  ;

  //------------------------------ Statistics values -----------------------------------------------------
  // Want this in the same scope as above so must be here
  class_<Statistics>("Statistics")
    .add_property("minimum", &Statistics::minimum, "Minimum value of the data set")
    .add_property("maximum", &Statistics::maximum, "Maximum value of the data set")
    .add_property("mean", &Statistics::mean, "Simple mean, sum(data)/nvalues, of the data set")
    .add_property("median", &Statistics::median, "Middle value of the data set")
    .add_property("standard_deviation", &Statistics::standard_deviation ,"Standard width of distribution")
  ;
}
