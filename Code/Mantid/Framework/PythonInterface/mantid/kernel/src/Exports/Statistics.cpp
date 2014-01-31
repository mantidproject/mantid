#include "MantidKernel/Statistics.h"
#include "MantidPythonInterface/kernel/Converters/NDArrayToVector.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/numeric.hpp>
#include <boost/python/overloads.hpp>
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
  /// Dummy class used to define Stats "namespace" in python
  class Stats {};

  /**
   * Proxy for getStatistics so that it can accept numpy arrays
   * @param data An input numpy array of data
   * @param sorted If true then the data is assumed to be sorted (default = false)
   * @returns A Statistics object describing the data
   */
  Statistics getStatisticsNumpy(const numeric::array & data, const bool sorted = false)
  {
    using Mantid::Kernel::getStatistics;

    // Need to extract actual type to go to C++. We'll limit this to C++ long (Python int) and
    // C++ double (Python float)
    // See http://docs.scipy.org/doc/numpy/reference/c-api.array.html#dealing-with-types
    if(PyArray_ISFLOAT(data.ptr()))
    {
      return getStatistics(Converters::NDArrayToVector<double>(data)(), sorted);
    }
    else if(PyArray_ISINTEGER(data.ptr()))
    {
      return getStatistics(Converters::NDArrayToVector<long>(data)(), sorted);
    }
    else
    {
      throw std::invalid_argument("getStatistics(): Unknown datatype. Currently only arrays of "
          "Python ints or floats are supported ");
    }
  }
  // Define an overload to handle the default argument
  BOOST_PYTHON_FUNCTION_OVERLOADS(getStatistics_Overloads, getStatisticsNumpy, 1, 2);

}

void export_Statistics()
{

 // define a new "Statistics" scope so that everything is called as Statistics.getXXX
  // this affects everything defined within the lifetime of the scope object
  scope stats = class_<Stats>("Stats",no_init)
     .def("getStatistics", &getStatisticsNumpy, getStatistics_Overloads(args("data", "sorted")))
     .staticmethod("getStatistics")
  ;

  // Statistics values holder
  class_<Statistics>("Statistics")
    .add_property("minimum", &Statistics::minimum, "Minimum value of the data set")
    .add_property("maximum", &Statistics::maximum, "Maximum value of the data set")
    .add_property("mean", &Statistics::mean, "Simple mean, sum(data)/nvalues, of the data set")
    .add_property("median", &Statistics::median, "Middle value of the data set")
    .add_property("standard_deviation", &Statistics::standard_deviation ,"Standard width of distribution")
  ;


}

