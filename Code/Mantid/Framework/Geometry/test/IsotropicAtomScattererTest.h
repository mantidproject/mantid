#ifndef MANTID_GEOMETRY_ISOTROPICATOMSCATTERERTEST_H_
#define MANTID_GEOMETRY_ISOTROPICATOMSCATTERERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/IsotropicAtomScatterer.h"

using namespace Mantid::Geometry;

class IsotropicAtomScattererTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IsotropicAtomScattererTest *createSuite() { return new IsotropicAtomScattererTest(); }
  static void destroySuite( IsotropicAtomScattererTest *suite ) { delete suite; }


  void test_Something()
  {
    TSM_ASSERT( "You forgot to write a test!", 0);
  }


};


#endif /* MANTID_GEOMETRY_ISOTROPICATOMSCATTERERTEST_H_ */
