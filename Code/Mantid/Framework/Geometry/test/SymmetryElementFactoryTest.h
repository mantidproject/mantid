#ifndef MANTID_GEOMETRY_SYMMETRYELEMENTFACTORYTEST_H_
#define MANTID_GEOMETRY_SYMMETRYELEMENTFACTORYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/SymmetryElementFactory.h"

using Mantid::Geometry::SymmetryElementFactory;
using namespace Mantid::API;

class SymmetryElementFactoryTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SymmetryElementFactoryTest *createSuite() { return new SymmetryElementFactoryTest(); }
  static void destroySuite( SymmetryElementFactoryTest *suite ) { delete suite; }


  void test_Something()
  {
    TSM_ASSERT( "You forgot to write a test!", 0);
  }


};


#endif /* MANTID_GEOMETRY_SYMMETRYELEMENTFACTORYTEST_H_ */