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

using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;

using std::runtime_error;
using std::size_t;
using std::vector;


//==========================================================================================
class EventWorkspaceTest : public CxxTest::TestSuite
{
private:
  EventWorkspace ew;

public:
  EventWorkspaceTest()
  {
  }

  void setUp()
  {
    ew.initialize(5000, 1001, 1000);
  }

  void test_constructor()
  {
    TS_ASSERT_EQUALS( ew.getNumberHistograms(), 5000);
    //std::cout << ew.size();
  }

  void test_notimplemented()
  {

    //ew.setX();
    //MantidVec mydata = ew.dataX(1);
    //TS_ASSERT_THROWS( MantidVec mydata = ew.dataX(1), Exception::NotImplementedError);
  }

};




#endif /* EVENTWORKSPACETEST_H_ */
