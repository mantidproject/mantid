#ifndef MANTID_API_LOGFILTERGENERATORTEST_H_
#define MANTID_API_LOGFILTERGENERATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/LogFilterGenerator.h"

using Mantid::API::LogFilterGenerator;

class LogFilterGeneratorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LogFilterGeneratorTest *createSuite() {
    return new LogFilterGeneratorTest();
  }
  static void destroySuite(LogFilterGeneratorTest *suite) { delete suite; }

  void test_Something() { TS_FAIL("You forgot to write a test!"); }
};

#endif /* MANTID_API_LOGFILTERGENERATORTEST_H_ */