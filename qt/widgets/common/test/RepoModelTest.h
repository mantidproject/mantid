// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTIDWIDGETS_REPOMODELTEST_H_
#define MANTIDWIDGETS_REPOMODELTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidQtWidgets/Common/RepoModel.h"

using MantidQt::API::RepoModel;

class RepoModelTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RepoModelTest *createSuite() { return new RepoModelTest(); }
  static void destroySuite(RepoModelTest *suite) { delete suite; }

  void test_Something() {

  }
};

#endif /* MANTIDWIDGETS_REPOMODELTEST_H_ */
