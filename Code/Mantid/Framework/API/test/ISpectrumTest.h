#ifndef MANTID_API_ISPECTRUMTEST_H_
#define MANTID_API_ISPECTRUMTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidAPI/ISpectrum.h"
#include "MantidTestHelpers/FakeObjects.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

class ISpectrumTest : public CxxTest::TestSuite
{
public:

  void test_empty_constructor()
  {
    SpectrumTester s;
    TS_ASSERT_EQUALS( s.getDetectorIDs().size(), 0);
    TS_ASSERT_EQUALS( s.getSpectrumNo(), 0);
  }

  void test_constructor()
  {
    SpectrumTester s(1234);
    TS_ASSERT_EQUALS( s.getDetectorIDs().size(), 0);
    TS_ASSERT_EQUALS( s.getSpectrumNo(), 1234);
  }

  void test_copyInfoFrom()
  {
    SpectrumTester a(1234);
    a.addDetectorID(678);
    a.addDetectorID(789);
    SpectrumTester b(456);

    TS_ASSERT_EQUALS( b.getDetectorIDs().size(), 0);
    b.copyInfoFrom(a);
    TS_ASSERT_EQUALS( b.getDetectorIDs().size(), 2);
    TS_ASSERT_EQUALS( b.getSpectrumNo(), 1234);
  }

  void test_setSpectrumNo()
  {
    SpectrumTester s;
    TS_ASSERT_EQUALS( s.getSpectrumNo(), 0);
    s.setSpectrumNo(1234);
    TS_ASSERT_EQUALS( s.getSpectrumNo(), 1234);
  }

  void test_detectorID_handling()
  {
    SpectrumTester s;
    TS_ASSERT( s.getDetectorIDs().empty() );
    s.addDetectorID(123);
    TS_ASSERT_EQUALS( s.getDetectorIDs().size(), 1);
    TS_ASSERT_EQUALS( *s.getDetectorIDs().begin(), 123);
    s.addDetectorID(456);
    s.addDetectorID(789);
    TS_ASSERT_EQUALS( s.getDetectorIDs().size(), 3);
    TS_ASSERT( s.hasDetectorID(123) );
    TS_ASSERT( s.hasDetectorID(456) );
    TS_ASSERT( s.hasDetectorID(789) );
    TS_ASSERT( !s.hasDetectorID(666) ); //No devil! ;)
    TS_ASSERT( !s.hasDetectorID(999) );

    int detids[] = {10,20,30,40,50,60,70,80,90};
    s.setDetectorIDs(std::set<detid_t>(detids,detids+3));
    TS_ASSERT_EQUALS( s.getDetectorIDs().size(), 3 );
    TS_ASSERT( s.hasDetectorID(20) );
    s.addDetectorIDs(std::set<detid_t>(detids+3,detids+6));
    TS_ASSERT_EQUALS( s.getDetectorIDs().size(), 6 );
    TS_ASSERT( s.hasDetectorID(20) );
    TS_ASSERT( s.hasDetectorID(60) );
    s.addDetectorIDs(std::set<detid_t>());
    TS_ASSERT_EQUALS( s.getDetectorIDs().size(), 6 );
    s.addDetectorIDs(std::vector<detid_t>(detids+4,detids+9)); // N.B. check unique elements
    TS_ASSERT_EQUALS( s.getDetectorIDs().size(), 9 );
    TS_ASSERT( s.hasDetectorID(10) );
    TS_ASSERT( s.hasDetectorID(70) );
    s.addDetectorIDs(std::vector<detid_t>());
    TS_ASSERT_EQUALS( s.getDetectorIDs().size(), 9 );

    s.clearDetectorIDs();
    TS_ASSERT( s.getDetectorIDs().empty() );

    s.addDetectorID(987);
    TS_ASSERT_EQUALS( s.getDetectorIDs().size(), 1);
    s.setDetectorIDs(std::set<detid_t>());
    TS_ASSERT( s.getDetectorIDs().empty() );
  }


};


#endif /* MANTID_API_ISPECTRUMTEST_H_ */

