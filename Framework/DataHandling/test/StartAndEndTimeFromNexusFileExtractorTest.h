#ifndef MANTID_DATAHANDLING_STARTANDENDTIMEFROMNEXUSFILEEXTRACTORTEST_H_
#define MANTID_DATAHANDLING_STARTANDENDTIMEFROMNEXUSFILEEXTRACTORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/StartAndEndTimeFromNexusFileExtractor.h"

using Mantid::DataHandling::StartAndEndTimeFromNexusFileExtractor;

class StartAndEndTimeFromNexusFileExtractorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static StartAndEndTimeFromNexusFileExtractorTest *createSuite() { return new StartAndEndTimeFromNexusFileExtractorTest(); }
  static void destroySuite( StartAndEndTimeFromNexusFileExtractorTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_DATAHANDLING_STARTANDENDTIMEFROMNEXUSFILEEXTRACTORTEST_H_ */