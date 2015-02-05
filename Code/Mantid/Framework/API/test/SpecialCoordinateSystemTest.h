#ifndef MANTID_API_SPECIALCOORDINATESYSTEMTEST
#define MANTID_API_SPECIALCOORDINATESYSTEMTEST

#include <cxxtest/TestSuite.h>
#include "MantidAPI/SpecialCoordinateSystem.h"

using namespace Mantid::API;

/*
 * We are testing the enum because the order of the elements in the enum is critical. There are various places in the codebase where the
 * enum integer values are assumed to be constant. A proper fix for this would be a type replacment enum->object, but while that has not been done,
 * a santiy check (these tests) on the enum ordering will prevent unintentional reordering.
*/
class SpecialCoordinateSystemTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor (& destructor) isn't called when running other tests
  static SpecialCoordinateSystemTest *createSuite() { return new SpecialCoordinateSystemTest(); }
  static void destroySuite( SpecialCoordinateSystemTest *suite ) { delete suite; }

  void test_none()
  {
      // Do not change
      SpecialCoordinateSystem none = None;
      TS_ASSERT_EQUALS(0, none);
  }

  void test_qLab()
  {
      // Do not change
      SpecialCoordinateSystem qlab = QLab;
      TS_ASSERT_EQUALS(1, qlab);
  }

  void test_qSample()
  {
      // Do not change
      SpecialCoordinateSystem qSample = QSample;
      TS_ASSERT_EQUALS(2, qSample);
  }

  void test_HKL()
  {
      // Do not change
      SpecialCoordinateSystem hkl = HKL;
      TS_ASSERT_EQUALS(3, hkl);
  }


 };

#endif


