#ifndef MANTID_DATAHANDLING_LOADSASSENATEST_H_
#define MANTID_DATAHANDLING_LOADSASSENATEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/LoadSassena.h"

class LoadSassenaTest : public CxxTest::TestSuite
{
public:
  void test_Init()
  {
    Mantid::DataHandling::LoadSassena alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

};

#endif // MANTID_DATAHANDLING_LOADSASSENATEST_H_
