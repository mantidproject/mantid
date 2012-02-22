#ifndef SIMULATEMDDATATEST_H_
#define SIMULATEMDDATATEST_H_

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <stdlib.h>
#include <iostream>
#include <boost/scoped_ptr.hpp> 
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidMDAlgorithms/SimulateMDD.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDBoxIterator.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"

using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;







class SimulateMDDTest : public CxxTest::TestSuite
{
private:
      std::string testWrkspc;
      std::string testWrkspc2;
      IMDEventWorkspace_sptr inMDwrkspc;


public:

    // create simple MDEventWorkspace of 2x2x2x2=16 points of unit signal,
    // unit error
    void testInit()
    {
      testWrkspc="testMDEWrksp";
      testWrkspc2="testMDEWrksp2";
    	// MDEventWorkspace in 4D with 2x2x2x2 boxes and one event of sig=1 err=1 per box
    	// Using MDEvent not MDLeanEvent but run and detector pointers are not set
    	// arguments are splits on each axis and min/max for each axis
    	boost::shared_ptr<Mantid::MDEvents::MDEventWorkspace<Mantid::MDEvents::MDEvent<4>,4> >
    	    outnew = MDEventsTestHelper::makeMDEWFull<4>(2,0.0,2.,1);
    	inMDwrkspc = outnew;
      TS_ASSERT_EQUALS(outnew->getNumDims(),4);
      TS_ASSERT_EQUALS(outnew->getNPoints(),16);

      TS_ASSERT_THROWS_NOTHING( AnalysisDataService::Instance().add(testWrkspc, outnew) );

      boost::shared_ptr<Mantid::MDEvents::MDEventWorkspace<Mantid::MDEvents::MDEvent<4>,4> >
                outnew2 = MDEventsTestHelper::makeMDEWFull<4>(2,0.0,2.,1);
    	IMDBox<MDEvent<4>,4> * out = outnew2->getBox();

    	// add 2 additional events to the first two boxes to make the data less uniform
    	double pos[4];
    	out->calcVolume();
    	outnew->refreshCache();
    	for (size_t i=0;i<4;i++)
    	  pos[i]=0.05;
    	uint16_t runIndex=5;
    	int32_t detectorId=7;
    	float signal=2.;
    	float errorsq=2.;
    	pos[0]=0.05;
    	MDEvent<4> ev0(signal,errorsq,runIndex,detectorId,pos);
    	pos[0]=1.5;
    	MDEvent<4> ev1(signal,errorsq,runIndex,detectorId,pos);
    	detectorId=4;
    	signal=4.;
    	errorsq=4.;
    	pos[0]=0.95;
    	MDEvent<4> ev2(signal,errorsq,runIndex,detectorId,pos);
    	pos[0]=1.99;
    	MDEvent<4> ev3(signal,errorsq,runIndex,detectorId,pos);
    	pos[0]=0.2;
    	signal=0.0;
    	errorsq=0.0;
    	MDEvent<4> ev4(signal,errorsq,runIndex,detectorId,pos);
    	out->addEvent(ev0);
    	out->addEvent(ev1);
    	out->addEvent(ev2);
    	out->addEvent(ev3);
    	out->addEvent(ev4);

    	// need to do this to update the signal values
    	outnew2->refreshCache();
    	TS_ASSERT_EQUALS(outnew2->getNumDims(),4);
    	TS_ASSERT_EQUALS(outnew2->getNPoints(),21);

    	TS_ASSERT_THROWS_NOTHING( AnalysisDataService::Instance().add(testWrkspc2, outnew2) );


    }

    /** TODO: Note, Janik Zikovsky, Oct 7, 2011: Update this test
     * to use new data structures. Key part of the test
     * commented out.
     */
    void testExecSimulate()
    {
        using namespace Mantid::MDAlgorithms;

        SimulateMDD alg;

        alg.initialize();
        alg.setPropertyValue("InputMDWorkspace",testWrkspc);
        alg.setPropertyValue("OutputMDWorkspace","test_out1");
        alg.setPropertyValue("BackgroundModel","QuadEnTrans");
        alg.setPropertyValue("BackgroundModelP1", "1.0" );
        alg.setPropertyValue("BackgroundModelP2", "0.1" );
        alg.setPropertyValue("BackgroundModelP3", "0.01" );
        alg.setPropertyValue("ForegroundModel","Simple cubic Heisenberg FM spin waves, DSHO, uniform damping");
        TS_ASSERT_THROWS_NOTHING(alg.execute());

        IMDEventWorkspace_sptr outMDwrkspc;
        TS_ASSERT_THROWS_NOTHING( outMDwrkspc =
            AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(testWrkspc) );
        TS_ASSERT_EQUALS( outMDwrkspc->getNPoints(),16);
        std::string res = alg.getPropertyValue("Residual");
        double resdble=atof(res.c_str());
        TS_ASSERT_DELTA(resdble,0.2601,1.e-4);

        // test bg Exponential model in energy transfer with same data
        alg.setPropertyValue("BackgroundModel","ExpEnTrans");
        alg.setPropertyValue("BackgroundModelP1", "1." );
        alg.setPropertyValue("BackgroundModelP2", "0.1" );
        alg.setPropertyValue("BackgroundModelP3", "4.0" );
        TS_ASSERT_THROWS_NOTHING(alg.execute());
        res = alg.getPropertyValue("Residual");
        resdble=atof(res.c_str());
        TS_ASSERT_DELTA(resdble,0.1000,1.e-4);


        alg.initialize();
        alg.setPropertyValue("InputMDWorkspace",testWrkspc2);
        alg.setPropertyValue("OutputMDWorkspace","test_out2");
        alg.setPropertyValue("BackgroundModel","QuadEnTrans");
        alg.setPropertyValue("BackgroundModelP1", "1.0" );
        alg.setPropertyValue("BackgroundModelP2", "0.1" );
        alg.setPropertyValue("BackgroundModelP3", "0.01" );
        TS_ASSERT_THROWS_NOTHING(alg.execute());
        res = alg.getPropertyValue("Residual");
        resdble=atof(res.c_str());
        TS_ASSERT_DELTA(resdble,3.6978,1.e-4);
    }
    void testTidyUp()
    {
      AnalysisDataService::Instance().remove(testWrkspc);
      AnalysisDataService::Instance().remove(testWrkspc2);
    }

};

#endif /*SIMULATEMDDATATEST_H_*/
