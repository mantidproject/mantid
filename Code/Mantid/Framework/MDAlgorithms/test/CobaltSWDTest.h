#ifndef COBALTSWDTEST_H_
#define COBALTSWDTEST_H_
//
#include <cxxtest/TestSuite.h>
//
#include "MantidMDAlgorithms/CobaltSpinWaveDSHO.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ProgressText.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FunctionFactory.h"
// from SimulateMDDTest
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDBoxIterator.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
//
#include "MantidDataObjects/WorkspaceSingleValue.h"
//
#include <iostream>
#include <boost/scoped_ptr.hpp>
//


using namespace Mantid;
//
using namespace Mantid::MDEvents;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::DataObjects;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::Geometry;
//
typedef Mantid::DataObjects::Workspace2D_sptr WS_type;
typedef Mantid::DataObjects::TableWorkspace_sptr TWS_type;
//
class CobaltSWDTest : public CxxTest::TestSuite
{
private:
  std::string testWrkspc;
  std::string testWrkspc2;
  std::string testWrkspc3;
  IMDEventWorkspace_sptr inMDwrkspc;
  boost::shared_ptr<Mantid::MDAlgorithms::RunParam> rParam2;
//
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CobaltSWDTest *createSuite() { return new CobaltSWDTest(); }
  static void destroySuite( CobaltSWDTest *suite ) { delete suite; }

    // create a test data set of 6 MDPoints contributing to 4 MDCells with 1, 2 and 3, 4 points each.
    CobaltSWDTest()
    {

        testWrkspc="testMDEWrksp";
        testWrkspc2="testMDEWrksp2";
        testWrkspc3="testMDEWrksp3";
        // MDEventWorkspace in 4D with 2x2x2x2 boxes and one event of sig=1 err=1 per box
        // Using MDEvent not MDLeanEvent but run and detector pointers are not set
        // arguments are splits on each axis and min/max for each axis
        boost::shared_ptr<Mantid::MDEvents::MDEventWorkspace<Mantid::MDEvents::MDEvent<4>,4> >
            outnew = MDEventsTestHelper::makeMDEWFull<4>(3,0.0,3.,1);
        inMDwrkspc = outnew;
        outnew->refreshCache();
        TS_ASSERT_EQUALS(outnew->getNumDims(),4);
        TS_ASSERT_EQUALS(outnew->getNPoints(),81);

        TS_ASSERT_THROWS_NOTHING( AnalysisDataService::Instance().add(testWrkspc, outnew) );

        // build another workspace with non uniform signal
        boost::shared_ptr<Mantid::MDEvents::MDEventWorkspace<Mantid::MDEvents::MDEvent<4>,4> >
                  outnew2 = MDEventsTestHelper::makeMDEWFull<4>(4,0.0,4.);
        //IMDBox<MDEvent<4>,4> * out = outnew2->getBox();

        // add additional events to make data quadratic in 4th coordinate (energy)
        //out->calcVolume();

        double pos[4];
        uint16_t runIndex=1;
        int32_t detectorId;
        float signal;
        float errorsq;
        size_t npt=4;
        ProgressText * prog = NULL;
        errorsq=1.0;
        std::vector<MDEvent<4> > events;
        for(size_t x = 0 ; x<npt ; x++ )
        {
          pos[0]=0.5+static_cast<double>(x);
          for( size_t y = 0 ; y<npt ; y++ )
          {
            pos[1]=0.5+static_cast<double>(y);
            for( size_t z = 0; z<npt ; z++ )
            {
              pos[2]=0.5+static_cast<double>(z);
              for( size_t e = 0; e<npt ; e++ )
              {
                double ep = 0.5+static_cast<double>(e);
                pos[3]=ep;
                signal=static_cast<float>(1.0+0.5*ep+0.1*ep*ep);
                detectorId=static_cast<int32_t>(e+1);
                events.push_back(MDEvent<4> (signal,errorsq,runIndex,detectorId,pos));
              }
            }
          }
        }
        outnew2->addManyEvents(events,prog);
        // need to do this to update the signal values
        outnew2->refreshCache();
        TS_ASSERT_EQUALS(outnew2->getNumDims(),4);
        TS_ASSERT_EQUALS(outnew2->getNPoints(),256);
        IMDIterator* it = outnew2->createIterator();
        TS_ASSERT_EQUALS( it->getDataSize() ,256);
        TS_ASSERT_EQUALS( it->getNumEvents() ,1);
        it->next();
        TS_ASSERT_EQUALS( it->getNumEvents() ,1);


        TS_ASSERT_THROWS_NOTHING( AnalysisDataService::Instance().add(testWrkspc2, outnew2) );

        // build third workspace with non uniform signal and noise
        boost::shared_ptr<Mantid::MDEvents::MDEventWorkspace<Mantid::MDEvents::MDEvent<4>,4> >
                  outnew3 = MDEventsTestHelper::makeMDEWFull<4>(4,0.0,4.);
        //IMDBox<MDEvent<4>,4> * out3 = outnew3->getBox();

        // add events to make data quadratic in 4th coordinate with noise
        //out3->calcVolume();

        errorsq=1.0;
        events.clear();
        double noise=0.1;
        for(size_t x = 0 ; x<npt ; x++ )
        {
          pos[0]=0.5+static_cast<double>(x);
          for( size_t y = 0 ; y<npt ; y++ )
          {
            pos[1]=0.5+static_cast<double>(y);
            for( size_t z = 0; z<npt ; z++ )
            {
              pos[2]=0.5+static_cast<double>(z);
              for( size_t e = 0; e<npt ; e++ )
              {
                double ep = 0.5+static_cast<double>(e);
                pos[3]=ep;
                signal=static_cast<float>(1.0+0.5*ep+0.1*ep*ep+noise*(double(rand())/double(RAND_MAX)-0.5));
                detectorId=static_cast<int32_t>(e+1);
                events.push_back(MDEvent<4> (signal,errorsq,runIndex,detectorId,pos));
              }
            }
          }
        }
        outnew3->addManyEvents(events,prog);
        // need to do this to update the signal values
        outnew3->refreshCache();
        TS_ASSERT_EQUALS(outnew3->getNumDims(),4);
        TS_ASSERT_EQUALS(outnew3->getNPoints(),256);
        IMDIterator* it3 = outnew3->createIterator();
        TS_ASSERT_EQUALS( it3->getDataSize() ,256);
        TS_ASSERT_EQUALS( it3->getNumEvents() ,1);
        it->next();
        TS_ASSERT_EQUALS( it3->getNumEvents() ,1);


        TS_ASSERT_THROWS_NOTHING( AnalysisDataService::Instance().add(testWrkspc3, outnew3) );
    }

    void testWithFit()
    {
        // test Fit - note that fit is to cell data but that MDCell
        // returns the sum of point contributions, not average.
        // As the number of points in a cell varies 1 to 4 this must be taken into
        // account if comparing the fit to the cell data.
        Fit alg1;
        TS_ASSERT_THROWS_NOTHING(alg1.initialize());
        TS_ASSERT( alg1.isInitialized() );

        // name of workspace to test against
        std::string wsName = testWrkspc;
        // RunParam for demo example
        rParam2 = boost::shared_ptr<RunParam> (new RunParam(
            45., 45., 5., 42.,
            0.5, 10., 7.19, 1.82,
            66.67, 66.67, 13.55314, 50.,
            0., 0., 0., 26.7,
            1, 2.28, 49., 1300.,
            150., 0., 3.87, 3.87,
            3.87, 90., 90., 90.,
            0., 0., 1.,  // u,v to Mantid z beam coords
            1., 0., 0.,  //
            0., 0., 0., 0.,
            1., 0., 1.,  //x,y to mantid z beam coords
            1., 0., -0., //
            10., 14., 18., 1,
            10., 0.5
            ));

        // set up fitting function
        CobaltSpinWaveDSHO* fn = new CobaltSpinWaveDSHO();
        API::IFunction_sptr fun(fn);

        fn->initialize();
        fn->setRunDataInfo(rParam2);

        //alg1.setPropertyValue("Function",fn->asString());


        // Set which spectrum to fit against and initial starting values
        alg1.setProperty("Function",fun);
        alg1.setPropertyValue("InputWorkspace", testWrkspc);

        // execute fit
        //TS_ASSERT_THROWS_NOTHING(
        //    TS_ASSERT( alg1.execute() )
        //    )

        //TS_ASSERT( alg1.isExecuted() );

        /*
        std::string algStat;
        algStat = alg1.getPropertyValue("OutputStatus");
        TS_ASSERT( algStat.compare("success")==0 );

        // test the output from fit is as expected - since 3 variables and 3 data points expect 0 Chi2
        double chisq = alg1.getProperty("OutputChi2overDoF");
        TS_ASSERT_DELTA( chisq, 0.0, 0.001 );

        IFunction_sptr out = FunctionFactory::Instance().createInitialized(alg1.getPropertyValue("Function"));
        TS_ASSERT_DELTA( out->getParameter("Constant"), 1.00 ,0.001);
        TS_ASSERT_DELTA( out->getParameter("Linear"), 0.00 ,0.001);
        TS_ASSERT_DELTA( out->getParameter("Quadratic"), 0.00 ,0.001);

        // test with 2nd workspace that has a signal quadratic in energy
        Fit alg2;
        TS_ASSERT_THROWS_NOTHING(alg2.initialize());
        TS_ASSERT( alg2.isInitialized() );

        // Set which spectrum to fit against and initial starting values
        alg2.setPropertyValue("Function",fn->asString());
        alg2.setPropertyValue("InputWorkspace", testWrkspc2);
        alg2.setPropertyValue("Output","out2");

        // execute fit
        TS_ASSERT_THROWS_NOTHING(
            TS_ASSERT( alg2.execute() )
            )
        TS_ASSERT( alg2.isExecuted() );
        algStat = alg2.getPropertyValue("OutputStatus");
        TS_ASSERT( algStat.compare("success")==0 );
        // test the output from fit is as expected - since 3 variables and 3 data points expect 0 Chi2
        chisq = alg2.getProperty("OutputChi2overDoF");
        TS_ASSERT_DELTA( chisq, 0.0, 0.001 );

        // there is no such workspace for Fit as far as I can tell
        //WS_type outWS = getWS("out3_Workspace");

        TWS_type outParams = getTWS("out2_Parameters");
        TS_ASSERT(outParams);
        TS_ASSERT_EQUALS(outParams->rowCount(),4);
        TS_ASSERT_EQUALS(outParams->columnCount(),3);

        TableRow row = outParams->getFirstRow();
        TS_ASSERT_EQUALS(row.String(0),"Constant");
        TS_ASSERT_DELTA(row.Double(1),1.00,0.001);

        row = outParams->getRow(1);
        TS_ASSERT_EQUALS(row.String(0),"Linear");
        TS_ASSERT_DELTA(row.Double(1),0.50,0.001);

        row = outParams->getRow(2);
        TS_ASSERT_EQUALS(row.String(0),"Quadratic");
        TS_ASSERT_DELTA(row.Double(1),0.10,0.001);

        // test with 3nd workspace that has a signal quadratic in energy plus noise
        Fit alg3;
        TS_ASSERT_THROWS_NOTHING(alg3.initialize());
        TS_ASSERT( alg3.isInitialized() );
        // Set which spectrum to fit against and initial starting values
        alg3.setPropertyValue("Function",fn->asString());
        alg3.setPropertyValue("InputWorkspace", testWrkspc3);
        alg3.setPropertyValue("Output","out3");

        // execute fit
        TS_ASSERT_THROWS_NOTHING(
            TS_ASSERT( alg3.execute() )
            )
        TS_ASSERT( alg3.isExecuted() );
        algStat = alg3.getPropertyValue("OutputStatus");
        TS_ASSERT( algStat.compare("success")==0 );
        // test the output from fit is as expected - since 3 variables and 3 data points expect 0 Chi2
        chisq = alg3.getProperty("OutputChi2overDoF");
        TS_ASSERT_DELTA( chisq, 0.0, 0.001 );

        // there is no such workspace for Fit as far as I can tell
        //WS_type outWS = getWS("out3_Workspace");

        TWS_type outParams3 = getTWS("out3_Parameters");
        TS_ASSERT(outParams3);
        TS_ASSERT_EQUALS(outParams3->rowCount(),4);
        TS_ASSERT_EQUALS(outParams3->columnCount(),3);

        row = outParams3->getFirstRow();
        TS_ASSERT_EQUALS(row.String(0),"Constant");
        TS_ASSERT_DELTA(row.Double(1),1.00,0.04);

        row = outParams3->getRow(1);
        TS_ASSERT_EQUALS(row.String(0),"Linear");
        TS_ASSERT_DELTA(row.Double(1),0.50,0.02);

        row = outParams3->getRow(2);
        TS_ASSERT_EQUALS(row.String(0),"Quadratic");
        TS_ASSERT_DELTA(row.Double(1),0.10,0.02);



        removeWS("out2_Parameters");
        removeWS("out3_Parameters");
        */

    }

    void testTidyUp()
    {
       removeWS(testWrkspc);
       removeWS(testWrkspc2);
       removeWS(testWrkspc3);
    }

    TWS_type getTWS(const std::string& name)
    {
        return AnalysisDataService::Instance().retrieveWS<Mantid::DataObjects::TableWorkspace>(name);
    }
    WS_type getWS(const std::string& name)
    {
      return AnalysisDataService::Instance().retrieveWS<Mantid::DataObjects::Workspace2D>(name);
    }
    void removeWS(const std::string& name)
    {
        AnalysisDataService::Instance().remove(name);
    }


};

#endif /*COBALTSWDTEST_H_*/

