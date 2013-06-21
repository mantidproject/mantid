#ifndef MANTID_CRYSTAL_PEAKSONSURFACETEST_H_
#define MANTID_CRYSTAL_PEAKSONSURFACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCrystal/PeaksOnSurface.h"

using Mantid::Crystal::PeaksOnSurface;

class PeaksOnSurfaceTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PeaksOnSurfaceTest *createSuite() { return new PeaksOnSurfaceTest(); }
  static void destroySuite( PeaksOnSurfaceTest *suite ) { delete suite; }


  void test_Init()
  {
    
  }


};


#endif /* MANTID_CRYSTAL_PEAKSONSURFACETEST_H_ */