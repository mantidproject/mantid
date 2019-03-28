#include <cxxtest/TestSuite.h>

static char const* const file = "filename";
static int line = 1;
//
// A test for issue #115: Defensive guards in ErrorFormatter to deal with
// (char*) nullptr being passed as an error message.
class NullPtrFormatterTest : public CxxTest::TestSuite {

  public:
  void testTsAssert()
  {
    CxxTest::doFailAssert(file, line, (char const*)NULL, (char const*)NULL);
  }
  void testTsFail() { CxxTest::doFailTest(file, line, (char const*)NULL); }
  void testTsWarn() { CxxTest::doWarn(file, line, (char const*)NULL); }
  void testTsSkip() { CxxTest::doSkipTest(file, line, (char const*)NULL); }
  void testTsTrace() { CxxTest::doTrace(file, line, (char const*)NULL); }
};
