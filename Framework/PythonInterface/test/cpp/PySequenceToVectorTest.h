// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef PYSEQUENCETOVECTORCONVERTERTEST_H_
#define PYSEQUENCETOVECTORCONVERTERTEST_H_

#include "MantidPythonInterface/core/Converters/PySequenceToVector.h"
#include <boost/python/dict.hpp>
#include <boost/python/list.hpp>
#include <boost/python/ssize_t.hpp>
#include <cxxtest/TestSuite.h>

using namespace Mantid::PythonInterface::Converters;

class PySequenceToVectorTest : public CxxTest::TestSuite {
public:
  static PySequenceToVectorTest *createSuite() {
    return new PySequenceToVectorTest();
  }
  static void destroySuite(PySequenceToVectorTest *suite) { delete suite; }

private:
  using PySequenceToVectorDouble = PySequenceToVector<double>;

public:
  void tearDown() override { PyErr_Clear(); }

  void test_construction_succeeds_with_a_valid_sequence_type() {
    boost::python::list testList;
    TS_ASSERT_THROWS_NOTHING(PySequenceToVectorDouble converter(testList));
  }

  void test_that_a_python_list_of_all_matching_types_is_converted_correctly() {
    boost::python::list testlist = createHomogeneousPythonList();
    const size_t ntestvals = boost::python::len(testlist);
    std::vector<double> cvector;
    TS_ASSERT_THROWS_NOTHING(cvector = PySequenceToVectorDouble(testlist)());
    // Test values
    TS_ASSERT_EQUALS(cvector.size(), ntestvals);
    // Check values
    for (size_t i = 0; i < ntestvals; ++i) {
      double testval = boost::python::extract<double>(testlist[i])();
      TS_ASSERT_EQUALS(cvector[i], testval);
    }
  }

  void
  test_that_trying_to_convert_a_list_of_incompatible_types_throws_error_already_set() {
    // Double->int is not generally safe so should not be allowed
    boost::python::list testlist = createHomogeneousPythonList();
    using PySequenceToVectorInt = PySequenceToVector<int>;
    std::vector<int> cvector;
    TS_ASSERT_THROWS(cvector = PySequenceToVectorInt(testlist)(),
                     const boost::python::error_already_set &);
  }

  /// Creates a python list where all of the types are the same
  static boost::python::list createHomogeneousPythonList() {
    boost::python::list testlist;
    const size_t ntestvals(20);
    for (size_t i = 0; i < ntestvals; ++i) {
      testlist.append((double)i + 10.0);
    }
    return testlist;
  }
};

#endif /* PYSEQUENCETOVECTORCONVERTERTEST_H_ */
