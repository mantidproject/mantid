// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTIDWIDGETS_FUNCTIONMODELTEST_H_
#define MANTIDWIDGETS_FUNCTIONMODELTEST_H_

#include "MantidQtWidgets/Common/FunctionModel.h"
#include <cxxtest/TestSuite.h>

using namespace MantidQt::MantidWidgets;

class FunctionModelTest : public CxxTest::TestSuite {

public:
  void test_empty() {
    FunctionModel model;
    TS_ASSERT(!model.isMultiDomain());
    TS_ASSERT_EQUALS(model.getNumberDomains(), 0);
    TS_ASSERT_EQUALS(model.currentDomainIndex(), 0);
    TS_ASSERT_THROWS_EQUALS(model.setCurrentDomainIndex(1), std::runtime_error &e, std::string(e.what()),
      "Domain index is out of range: 1 out of 0");
  }
};

#endif // MANTIDWIDGETS_FUNCTIONMODELTEST_H_
