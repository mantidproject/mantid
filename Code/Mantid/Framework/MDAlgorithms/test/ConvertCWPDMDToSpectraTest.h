#ifndef MANTID_MDALGORITHMS_CONVERTCWPDMDTOSPECTRATEST_H_
#define MANTID_MDALGORITHMS_CONVERTCWPDMDTOSPECTRATEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidMDAlgorithms/ConvertCWPDMDToSpectra.h"

using Mantid::MDAlgorithms::ConvertCWPDMDToSpectra;
using namespace Mantid::API;

class ConvertCWPDMDToSpectraTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertCWPDMDToSpectraTest *createSuite() {
    return new ConvertCWPDMDToSpectraTest();
  }
  static void destroySuite(ConvertCWPDMDToSpectraTest *suite) { delete suite; }

  void test_Something() { TSM_ASSERT("You forgot to write a test!", 0); }
};

#endif /* MANTID_MDALGORITHMS_CONVERTCWPDMDTOSPECTRATEST_H_ */