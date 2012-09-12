#ifndef MANTID_DATAHANDLING_SAVEFullprofRESOLUTIONTEST_H_
#define MANTID_DATAHANDLING_SAVEFullprofRESOLUTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/SaveFullprofResolution.h"

using Mantid::DataHandling::SaveFullprofResolution;

class SaveFullprofResolutionTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveFullprofResolutionTest *createSuite() { return new SaveFullprofResolutionTest(); }
  static void destroySuite( SaveFullprofResolutionTest *suite ) { delete suite; }


  void test_Init()
  {
    Mantid::DataHandling::SaveFullprofResolution alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }


};


#endif /* MANTID_DATAHANDLING_SAVEFullprofRESOLUTIONTEST_H_ */
