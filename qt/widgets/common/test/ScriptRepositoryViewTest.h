// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTIDQT_API_SCRIPTREPOSITORYVIEWTEST_H_
#define MANTIDQT_API_SCRIPTREPOSITORYVIEWTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidQtWidgets/Common/ScriptRepositoryView.h"

using MantidQt::API::ScriptRepositoryView;

class ScriptRepositoryViewTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ScriptRepositoryViewTest *createSuite() { return new ScriptRepositoryViewTest(); }
  static void destroySuite( ScriptRepositoryViewTest *suite ) { delete suite; }


  void test_Something()
  {
		ScriptRepositoryView();
  }


};


#endif /* MANTIDQT_API_SCRIPTREPOSITORYVIEWTEST_H_ */