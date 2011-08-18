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
    boost::shared_ptr<Instrument> i = ws.getInstrument();
    TSM_ASSERT( "ExperimentInfo gets a default, empty Instrument.", i);
    TS_ASSERT_EQUALS( ws.getInstrument()->type(), "Instrument" );
  }

  void test_GetSetInstrument_default()
  {
    ExperimentInfo ws;
    boost::shared_ptr<Instrument> inst1(new Instrument());
    inst1->setName("MyTestInst");
    ws.setInstrument(inst1);

    // Instruments don't point to the same base place since they
    boost::shared_ptr<Instrument> inst2 = ws.getInstrument();
    TS_ASSERT_EQUALS( inst2->getName(), "MyTestInst");

    // But the base instrument does!
    boost::shared_ptr<Instrument> inst3 = ws.getBaseInstrument();
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



  void do_compare_ExperimentInfo(ExperimentInfo & ws, ExperimentInfo & ws2)
  {
    TS_ASSERT_EQUALS( ws2.sample().getName(), "test" );
    TS_ASSERT_DELTA( ws2.sample().getOrientedLattice().a(), 1.0, 1e-4);
    TS_ASSERT_DELTA( ws2.sample().getOrientedLattice().b(), 2.0, 1e-4);
    TS_ASSERT_DELTA( ws2.sample().getOrientedLattice().c(), 3.0, 1e-4);
    TS_ASSERT_DELTA( ws2.run().getProtonCharge(), 1.234, 0.001);
    TS_ASSERT_EQUALS( ws2.getInstrument()->getName(), "MyTestInst");

    // Changing stuff in the original workspace...
    ws.mutableSample().setName("test1");
    ws.mutableRun().setProtonCharge(2.345);

    // ... does not change the copied one.
    TS_ASSERT_EQUALS( ws2.sample().getName(), "test" );
    TS_ASSERT_DELTA( ws2.run().getProtonCharge(), 1.234, 0.001);

    // The original oriented lattice is ok
    TS_ASSERT_DELTA( ws.sample().getOrientedLattice().a(), 1.0, 1e-4);
    TS_ASSERT_DELTA( ws.sample().getOrientedLattice().b(), 2.0, 1e-4);
    TS_ASSERT_DELTA( ws.sample().getOrientedLattice().c(), 3.0, 1e-4);
  }


  void test_copyExperimentInfoFrom()
  {
    ExperimentInfo ws;
    ws.mutableRun().setProtonCharge(1.234);
    ws.mutableSample().setName("test");
    ws.mutableSample().setOrientedLattice( new OrientedLattice(1,2,3,90,90,90) );
    boost::shared_ptr<Instrument> inst1(new Instrument());
    inst1->setName("MyTestInst");
    ws.setInstrument(inst1);

    ExperimentInfo ws2;
    ws2.copyExperimentInfoFrom( &ws );
    do_compare_ExperimentInfo(ws,ws2);
  }

  void test_clone()
  {
    ExperimentInfo ws;
    ws.mutableRun().setProtonCharge(1.234);
    ws.mutableSample().setName("test");
    ws.mutableSample().setOrientedLattice( new OrientedLattice(1,2,3,90,90,90) );
    boost::shared_ptr<Instrument> inst1(new Instrument());
    inst1->setName("MyTestInst");
    ws.setInstrument(inst1);

    ExperimentInfo * ws2 = ws.cloneExperimentInfo();
    do_compare_ExperimentInfo(ws,*ws2);
  }


};


#endif /* MANTID_API_EXPERIMENTINFOTEST_H_ */

