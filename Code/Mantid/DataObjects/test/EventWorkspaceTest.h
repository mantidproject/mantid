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

public:
  EventWorkspaceTest()
  {
  }

  void setUp()
  {
  }

  void test_constructor()
  {
    ;
  }

};




#endif /* EVENTWORKSPACETEST_H_ */
