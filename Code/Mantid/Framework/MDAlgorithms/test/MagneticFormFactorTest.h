#ifndef MANTID_MDALGORITHMS_MAGNETICFORMFACTORTEST_H_
#define MANTID_MDALGORITHMS_MAGNETICFORMFACTORTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDAlgorithms/MagneticFormFactor.h"

using namespace Mantid;
using namespace Mantid::MDAlgorithms;

class MagneticFormFactorTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MagneticFormFactorTest *createSuite() { return new MagneticFormFactorTest(); }
  static void destroySuite( MagneticFormFactorTest *suite ) { delete suite; }

  MagneticFormFactor *mag;
  void test_Init()
  {
    mag = new MagneticFormFactor(25,0,1000);
  }

  void test_form()
  {
    TS_ASSERT_THROWS_NOTHING( mag->setFormFactor(25,3,500) )
    TS_ASSERT_DELTA( mag->form(6.48), 0.691958098, 1e-5);
    TS_ASSERT_DELTA( mag->formTable(6.48), 0.692023, 1e-3)
    TS_ASSERT_DELTA( mag->form(500.), 0.0, 1e-5)
    TS_ASSERT_THROWS_NOTHING( mag->setFormFactor(0,0,500) )
    TS_ASSERT_DELTA( mag->form(2.1), 1.0, 1e-5)
  }


};


#endif /* MANTID_MDALGORITHMS_MAGNETICFORMFACTORTEST_H_ */
