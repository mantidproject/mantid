#ifndef PYSEQUENCETOVECTORCONVERTERTEST_H_
#define PYSEQUENCETOVECTORCONVERTERTEST_H_

#include "MantidPythonInterface/kernel/Converters/PySequenceToVectorConverter.h"
#include <boost/python/ssize_t.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/list.hpp>
#include <cxxtest/TestSuite.h>

using namespace Mantid::PythonInterface::Converters;

class PySequenceToVectorConverterTest : public CxxTest::TestSuite
{
private:
  typedef PySequenceToVectorConverter<double> PySequenceToVectorDouble;
public:

  void test_construction_succeeds_with_a_valid_sequence_type()
  {
    boost::python::list testList;
    TS_ASSERT_THROWS_NOTHING(PySequenceToVectorDouble converter(testList));
  }

  void test_construction_throws_when_not_given_a_PySequence()
  {
    boost::python::dict testDict;
    TS_ASSERT_THROWS(PySequenceToVectorDouble converter(testDict), std::invalid_argument);
  }

  void test_that_a_python_list_of_all_matching_types_is_converted_correctly()
  {
    boost::python::list testlist = createHomogeneousPythonList();
    const size_t ntestvals = boost::python::len(testlist);
    std::vector<double> cvector;
    TS_ASSERT_THROWS_NOTHING(cvector = PySequenceToVectorDouble(testlist)());
    //Test values
    TS_ASSERT_EQUALS(cvector.size(), ntestvals);
    // Check values
    for( size_t i = 0; i < ntestvals; ++i)
    {
      double testval = boost::python::extract<double>(testlist[i])();
      TS_ASSERT_EQUALS(cvector[i], testval);
    }
  }

  void test_that_trying_to_convert_a_list_of_incompatible_types_throws_error_already_set()
  {
    // Double->int is not generally safe so should not be allowed
    boost::python::list testlist = createHomogeneousPythonList();
    typedef PySequenceToVectorConverter<int> PySequenceToVectorInt;
    std::vector<int> cvector;
    TS_ASSERT_THROWS(cvector = PySequenceToVectorInt(testlist)(), boost::python::error_already_set);
  }

  /// Creates a python list where all of the types are the same
  static boost::python::list createHomogeneousPythonList()
  {
    boost::python::list testlist;
    const size_t ntestvals(20);
    for( size_t i = 0; i < ntestvals; ++i)
    {
      testlist.append((double)i+10.0);
    }
    return testlist;
  }
};


#endif /* PYSEQUENCETOVECTORCONVERTERTEST_H_ */
