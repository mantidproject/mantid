/*
 * EventWorkspaceTest.h
 *
 *  Created on: May 28, 2010
 *      Author: Janik Zikovsky
 */

#ifndef EVENTWORKSPACETEST_H_
#define EVENTWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"

using namespace Mantid::DataObjects;

using std::runtime_error;
using std::size_t;
using std::vector;


//==========================================================================================
class EventWorkspaceTest : public CxxTest::TestSuite
{
private:
  EventWorkspace ev;

public:
  EventWorkspaceTest()
  {
  }

  void setUp()
  {
    ev.init(5000, 1001, 1000);
  }

  void test_constructor()
  {
    TS_ASSERT_EQUALS( ev.getNumberHistograms(), 5000);
  }

};




#endif /* EVENTWORKSPACETEST_H_ */
