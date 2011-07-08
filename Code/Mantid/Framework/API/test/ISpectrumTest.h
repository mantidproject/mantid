#ifndef MANTID_API_ISPECTRUMTEST_H_
#define MANTID_API_ISPECTRUMTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidAPI/ISpectrum.h"
#include "FakeObjects.h"

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
    TS_ASSERT_EQUALS( s.getDetectorIDs().size(), 0);
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

    s.clearDetectorIDs();
    TS_ASSERT_EQUALS( s.getDetectorIDs().size(), 0);
  }


};


#endif /* MANTID_API_ISPECTRUMTEST_H_ */

