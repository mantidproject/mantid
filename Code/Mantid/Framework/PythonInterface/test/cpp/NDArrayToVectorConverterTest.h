#ifndef NDARRAYTOVECTORCONVETERTEST_H_
#define NDARRAYTOVECTORCONVETERTEST_H_

#include "MantidPythonInterface/kernel/Converters/NDArrayToVectorConverter.h"
#include <boost/python/detail/prefix.hpp> // Include Python.h safely
#define PY_ARRAY_UNIQUE_SYMBOL KERNEL_ARRAY_API
#include <numpy/ndarrayobject.h> // _import_array

#include <boost/python/list.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/numeric.hpp>

#include <cxxtest/TestSuite.h>
#include "PySequenceToVectorConverterTest.h" // Use the helper methods here
#include <iomanip>

using namespace Mantid::PythonInterface;

class NDArrayToVectorConverterTest : public CxxTest::TestSuite
{

private:
  /// Typedef of numpy array -> std::vector double
  typedef NDArrayToVectorConverter<double> NumpyToVectorDouble;

public:

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NDArrayToVectorConverterTest *createSuite() { return new NDArrayToVectorConverterTest(); }
  static void destroySuite( NDArrayToVectorConverterTest *suite ) { delete suite; }

  NDArrayToVectorConverterTest()
  {
    _import_array(); // Import the numpy module for this process
    boost::python::numeric::array::set_module_and_type("numpy", "ndarray");
  }

  void test_construction_succeeds_with_a_numpy_array()
  {
    boost::python::list testList;
    boost::python::numeric::array testArray(testList);
    TS_ASSERT_THROWS_NOTHING(NumpyToVectorDouble converter(testArray));
  }

  void test_construction_throws_when_not_given_a_numpy_ndarray()
  {
    boost::python::list testList;
    TS_ASSERT_THROWS(NumpyToVectorDouble converter(testList), std::invalid_argument);
  }

  void test_1D_array_is_converted_correctly()
  {
    boost::python::list testvalues = PySequenceToVectorConverterTest::createHomogeneousPythonList();
    const size_t ntestvals(boost::python::len(testvalues));
    boost::python::numeric::array nparray(testvalues);

    std::vector<double> cvector;
    TS_ASSERT_THROWS_NOTHING(cvector = NumpyToVectorDouble(nparray)());
    TS_ASSERT_EQUALS(cvector.size(), ntestvals);
    // Check values
    for( size_t i = 0; i < ntestvals; ++i)
    {
      double testval = boost::python::extract<double>(testvalues[i])();
      TS_ASSERT_EQUALS(cvector[i], testval);
    }
  }

  void test_2D_array_is_converted_correctly()
  {
    std::vector<Py_ssize_t> dims;
    boost::python::numeric::array testArray = create2DArray(dims);
    const size_t nelements = PyArray_SIZE(testArray.ptr());
    std::vector<double> cvector;
    TS_ASSERT_THROWS_NOTHING(cvector = NumpyToVectorDouble(testArray)());
    TS_ASSERT_EQUALS(cvector.size(), nelements);

    // Check values
    for( Py_ssize_t row = 0; row < dims[0]; ++row)
    {
      Py_ssize_t offset;
      if( row == 0 ) offset = 0;
      else offset = (dims[1]);
      // Check values
      for( Py_ssize_t col = 0; col < dims[1]; ++col)
      {
        const Py_ssize_t pos = offset + col;
        const double expected = boost::python::extract<double>(testArray[row][col])();
        TS_ASSERT_EQUALS(cvector[pos], expected);
      }
    }
  }

  void test_double_vector_can_be_extracted_from_int()
  {
    boost::python::list testints;
    const int nelements(10);
    for( int i = 0; i < nelements; ++i)
    {
      testints.append(i+1);
    }
    boost::python::numeric::array nparray(testints);

    std::vector<double> cvector;
    cvector = NumpyToVectorDouble(nparray)();
    for( int i = 0; i < nelements; ++i)
    {
      double expected = boost::python::extract<double>(testints[i]);
      TS_ASSERT_EQUALS(cvector[i], expected);
    }
  }

  void test_vector_can_be_converted_to_string()
  {
    boost::python::list testvalues = PySequenceToVectorConverterTest::createHomogeneousPythonList();
    const size_t ntestvals(boost::python::len(testvalues));
    boost::python::numeric::array nparray(testvalues);


    typedef NDArrayToVectorConverter<std::string> NumpyToVectorString;
    std::vector<std::string> cvector = NumpyToVectorString(nparray)();
    TS_ASSERT_EQUALS(cvector.size(), ntestvals);
    // Check values
    for( size_t i = 0; i < ntestvals; ++i)
    {
      double testval = boost::python::extract<double>(testvalues[i])();
      double from_vector;
      std::istringstream is(cvector[i]);
      is >> from_vector;
      TS_ASSERT_EQUALS(testval, from_vector);
    }
  }


private:
  /// Creates a 2D array and populates the dims array with the dimensions
  boost::python::numeric::array create2DArray(std::vector<Py_ssize_t> & dims)
  {
    boost::python::list rowOne = PySequenceToVectorConverterTest::createHomogeneousPythonList();
    boost::python::list rowTwo = PySequenceToVectorConverterTest::createHomogeneousPythonList();
    dims.resize(2);
    dims[0] = 2;
    dims[1] = boost::python::len(rowOne);
    boost::python::list matrix;
    matrix.append(rowOne);
    matrix.append(rowTwo);
    return boost::python::numeric::array(matrix);
  }

};


#endif /* NDARRAYTOVECTORCONVETERTEST_H_ */
