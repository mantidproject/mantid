#ifndef MANTID_API_EXPERIMENTINFOTEST_H_
#define MANTID_API_EXPERIMENTINFOTEST_H_

#include "MantidAPI/ExperimentInfo.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid::API;
using namespace Mantid::Geometry;

class ExperimentInfoTest : public CxxTest::TestSuite
{
public:


  void test_GetInstrument_default()
  {
    ExperimentInfo ws;
    boost::shared_ptr<IInstrument> i = ws.getInstrument();
    TSM_ASSERT( "ExperimentInfo gets a default, empty Instrument.", i);
    TS_ASSERT_EQUALS( ws.getInstrument()->type(), "Instrument" );
  }

  void test_GetSetInstrument_default()
  {
    ExperimentInfo ws;
    boost::shared_ptr<IInstrument> inst1(new Instrument());
    inst1->setName("MyTestInst");
    ws.setInstrument(inst1);

    // Instruments don't point to the same base place since they
    boost::shared_ptr<IInstrument> inst2 = ws.getInstrument();
    TS_ASSERT_EQUALS( inst2->getName(), "MyTestInst");

    // But the base instrument does!
    boost::shared_ptr<IInstrument> inst3 = ws.getBaseInstrument();
    TS_ASSERT_EQUALS( inst3.get(), inst1.get());
    TS_ASSERT_EQUALS( inst3->getName(), "MyTestInst");
  }

  void test_GetSetSample()
  {
    ExperimentInfo ws;
    TS_ASSERT( &ws.sample() );
    ws.mutableSample().setName("test");
    TS_ASSERT_EQUALS( ws.sample().getName(), "test" );
  }

  void test_GetSetRun()
  {
    ExperimentInfo ws;
    TS_ASSERT( &ws.run() );
    ws.mutableRun().setProtonCharge(1.234);
    TS_ASSERT_DELTA( ws.run().getProtonCharge(), 1.234, 0.001);
  }


};


#endif /* MANTID_API_EXPERIMENTINFOTEST_H_ */

