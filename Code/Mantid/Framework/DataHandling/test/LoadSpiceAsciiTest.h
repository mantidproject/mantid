#ifndef MANTID_DATAHANDLING_LOADSPICEASCIITEST_H_
#define MANTID_DATAHANDLING_LOADSPICEASCIITEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadSpiceAscii.h"

using Mantid::DataHandling::LoadSpiceAscii;
using namespace Mantid::API;

class LoadSpiceAsciiTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadSpiceAsciiTest *createSuite() { return new LoadSpiceAsciiTest(); }
  static void destroySuite( LoadSpiceAsciiTest *suite ) { delete suite; }


  void test_Something()
  {
    TSM_ASSERT( "You forgot to write a test!", 0);
  }


};


#endif /* MANTID_DATAHANDLING_LOADSPICEASCIITEST_H_ */