// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTIDWIDGETS_FUNCTIONMULTIDOMAINPRENTERTEST_H_
#define MANTIDWIDGETS_FUNCTIONMULTIDOMAINPRENTERTEST_H_

#include "MantidQtWidgets/Common/FunctionModel.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IFunction.h"
#include <cxxtest/TestSuite.h>

using namespace MantidQt::MantidWidgets;
using namespace Mantid::API;

class FunctionMultiDomainPrenterTest : public CxxTest::TestSuite {

public:
  static FunctionMultiDomainPrenterTest *createSuite() {
    return new FunctionMultiDomainPrenterTest;
  }
  static void destroySuite(FunctionMultiDomainPrenterTest *suite) { delete suite; }

  FunctionMultiDomainPrenterTest() {
    // To make sure API is initialized properly
    FrameworkManager::Instance();
  }

  void test_empty() {
    SingleDomainFunctionModel model;
    TS_ASSERT(!model.getFitFunction());
  }
};

#endif // MANTIDWIDGETS_FUNCTIONMULTIDOMAINPRENTERTEST_H_
