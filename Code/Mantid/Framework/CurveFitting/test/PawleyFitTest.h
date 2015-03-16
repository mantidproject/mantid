#ifndef MANTID_CURVEFITTING_PAWLEYFITTEST_H_
#define MANTID_CURVEFITTING_PAWLEYFITTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/PawleyFit.h"

using Mantid::CurveFitting::PawleyFit;
using namespace Mantid::API;

class PawleyFitTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PawleyFitTest *createSuite() { return new PawleyFitTest(); }
  static void destroySuite( PawleyFitTest *suite ) { delete suite; }


  void test_Something()
  {
    TSM_ASSERT( "You forgot to write a test!", 0);
  }


};


#endif /* MANTID_CURVEFITTING_PAWLEYFITTEST_H_ */