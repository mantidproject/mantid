#ifndef MANTID_API_SCRIPTREPOSITORYVIEWTEST_H_
#define MANTID_API_SCRIPTREPOSITORYVIEWTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/ScriptRepositoryView.h"

using Mantid::API::ScriptRepositoryView;

class ScriptRepositoryViewTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ScriptRepositoryViewTest *createSuite() {
    return new ScriptRepositoryViewTest();
  }
  static void destroySuite(ScriptRepositoryViewTest *suite) { delete suite; }

  void test_Something() { TSM_ASSERT("You forgot to write a test!", 0); }
};

#endif /* MANTID_API_SCRIPTREPOSITORYVIEWTEST_H_ */