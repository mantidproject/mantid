#ifndef VECTORDELEGATETEST_H_
#define VECTORDELEGATETEST_H_

#include "MantidPythonInterface/kernel/VectorDelegate.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::PythonInterface;

class VectorDelegateTest : public CxxTest::TestSuite
{
public:

  void test_that_a_non_sequence_type_returns_an_appropriate_error_string_from_isConvertible()
  {
    PyObject *dict = PyDict_New();
    TS_ASSERT_EQUALS(VectorDelegate::isSequenceType(dict), "Cannot convert dict object to a std::vector.");
    Py_DECREF(dict);
  }

  void test_that_a_non_sequence_type_throws_an_error_when_trying_to_convert_to_a_vector()
  {
    PyObject *dict = PyDict_New();
    TS_ASSERT_THROWS(VectorDelegate::toStdVector<int>(dict), std::invalid_argument);
    Py_DECREF(dict);
  }

  void test_that_a_python_list_of_all_matching_types_is_converted_correctly()
  {
    const size_t length(3);
    boost::python::object lst = createPyIntList((Py_ssize_t)length, false);
    std::vector<int> cppvec = VectorDelegate::toStdVector<int>(lst.ptr());
    TS_ASSERT_EQUALS(cppvec.size(), length);
    // Check values
    for( size_t i = 0; i < length; ++i )
    {
      TS_ASSERT_EQUALS(cppvec[i], i);
    }
  }

  void test_that_trying_to_convert_a_python_list_of_differing_types_throws_error_already_set()
  {
    const size_t length(4);
    boost::python::object lst = createPyIntList((Py_ssize_t)length, true);
    TS_ASSERT_THROWS(VectorDelegate::toStdVector<int>(lst.ptr()), boost::python::error_already_set);
  }

private:

  /// Create a Python list of the given length filled with integer values
  /// v_i+1=v_i+1 starting at 0. If addItemOfDiffType is true add
  /// a value of the second type
  boost::python::object createPyIntList(Py_ssize_t length, bool addItemOfDiffType)
  {
    if(addItemOfDiffType) length += 1;
    PyObject *lst = PyList_New((Py_ssize_t)length);
    for(Py_ssize_t i = 0; i < length; ++i)
    {
      PyList_SetItem(lst, i, PyInt_FromLong((int)i)); // Cast avoids RHEL5 problem that doesn't have size_t
    }
    if(addItemOfDiffType)
    {
      PyList_SetItem(lst, length, PyFloat_FromDouble(100.0));
    }
    return boost::python::object(boost::python::handle<>(lst));
  }
};


#endif /* VECTORDELEGATETEST_H_ */
