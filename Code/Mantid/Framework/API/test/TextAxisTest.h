#ifndef TEXTAXISTEST_H_
#define TEXTAXISTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/TextAxis.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/System.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

// Now the unit test class itself
class TextAxisTest : public CxxTest::TestSuite
{
public:

  void testConstructor()
  {
    TextAxis ta(3);
    TS_ASSERT_EQUALS(ta.length(),3);
    TS_ASSERT(ta.unit());
    TS_ASSERT_EQUALS(ta(0),Mantid::EMPTY_DBL());
    TS_ASSERT_THROWS(ta.setValue(0,10.),std::domain_error);
    TS_ASSERT(ta.isText());
  }

  void testLabels()
  {
    TextAxis ta(3);
    ta.setLabel(0,"First");
    ta.setLabel(1,"Second");
    ta.setLabel(2,"Third");

    TS_ASSERT_EQUALS(ta.label(0),"First");
    TS_ASSERT_EQUALS(ta.label(1),"Second");
    TS_ASSERT_EQUALS(ta.label(2),"Third");
  }

  void testEquals()
  {
    TextAxis ta1(2);
    ta1.setLabel(0,"First");
    ta1.setLabel(1,"Second");

    TextAxis ta2(2);
    ta2.setLabel(0,"First");
    ta2.setLabel(1,"Second");

    TextAxis ta3(3);
    ta3.setLabel(0,"First");
    ta3.setLabel(1,"Second");
    ta3.setLabel(2,"Third");

    TextAxis ta4(2);
    ta4.setLabel(0,"Second");
    ta4.setLabel(1,"First");

    TS_ASSERT(ta1 == ta2);
    TS_ASSERT( !(ta1 == ta3) );
    TS_ASSERT( !(ta2 == ta4) );
  }

  void testClone()
  {
    TextAxis ta1(2);
    ta1.setLabel(0,"First");
    ta1.setLabel(1,"Second");

    Axis* a2 = ta1.clone();
    TS_ASSERT(a2);
    if (a2)
    {
      TS_ASSERT(ta1 == *a2);
      delete a2;
    }
  }


};

#endif /*TEXTAXISTEST_H_*/
