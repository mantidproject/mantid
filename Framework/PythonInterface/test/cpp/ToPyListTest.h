// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_TOPYLISTTEST_H
#define MANTID_PYTHONINTERFACE_TOPYLISTTEST_H

#include "MantidPythonInterface/core/Converters/ToPyList.h"
#include <boost/python/errors.hpp>
#include <cxxtest/TestSuite.h>

using namespace Mantid::PythonInterface::Converters;

class ToPyListTest : public CxxTest::TestSuite {
public:
  static ToPyListTest *createSuite() { return new ToPyListTest(); }
  static void destroySuite(ToPyListTest *suite) { delete suite; }

  using ToPyListVectorDouble = ToPyList<double>;

  void test_empty_vector_returns_empty_list() {
    std::vector<double> empty;
    boost::python::list result;
    TS_ASSERT_THROWS_NOTHING(result = ToPyListVectorDouble()(empty));
    TS_ASSERT_EQUALS(0, boost::python::len(result));
  }

  void test_unregistered_element_type_throws_runtime_error() {
    std::vector<UnregisteredType> unknownElements{UnregisteredType()};
    TS_ASSERT_THROWS(ToPyList<UnregisteredType>()(unknownElements),
                     const boost::python::error_already_set &);
  }

private:
  struct UnregisteredType {};
};

#endif // MANTID_PYTHONINTERFACE_TOPYLISTTEST_H
