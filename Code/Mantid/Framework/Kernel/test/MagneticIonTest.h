#ifndef MAGNETICIONTEST_H_
#define MAGNETICIONTEST_H_

#include <cxxtest/TestSuite.h>
#include <stdexcept>
#include "MantidKernel/MagneticIon.h"

using namespace Mantid::PhysicalConstants;

class MagneticIonTest : public CxxTest::TestSuite
{
public:
  void testGetMagneticIon()
  {
    MagneticIon temp;
    temp=getMagneticIon("Am",7);
    TS_ASSERT_EQUALS(temp.symbol,"Am");
    TS_ASSERT_EQUALS(temp.charge,7);
    TS_ASSERT_DELTA(temp.j0[1],12.73,0.001);
  }

  void testGetJL()
  {
    std::vector <double> temp;
    temp=getJL("Am",7);
    TS_ASSERT_EQUALS(temp.size(),8);
    TS_ASSERT_DELTA(temp[1],12.73,0.001);
  }

  void testErrors()
  {
    TS_ASSERT_THROWS(getMagneticIon("O",2), std::runtime_error);    //no such ion
    TS_ASSERT_THROWS(getMagneticIon("Am",12), std::runtime_error);  //no such charge
    TS_ASSERT_THROWS(getJL("Am",12), std::runtime_error);           //no such charge - pass to getJL
    TS_ASSERT_THROWS(getJL("Am",7,3), std::runtime_error);          //no such l 
  }
};

#endif // MAGNETICIONTEST_H_
