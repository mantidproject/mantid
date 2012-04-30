#ifndef DAMPEDHISENBERGTEST_H_
#define DAMPEDHISENBERGTEST_H_
//
#include <cxxtest/TestSuite.h>
//
#include "MantidMDAlgorithms/DampedHisenbergFMSW.h"
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
//
// Class TestDampedHisenbergFMSW to get access to functionMD
//
namespace Mantid
{
  namespace MDAlgorithms
  {
    class DLLExport TestDampedHisenbergFMSW : public DampedHisenbergFMSW
    {
    public:
      TestDampedHisenbergFMSW() : DampedHisenbergFMSW()
      {
      };
      /// Destructor
      virtual ~TestDampedHisenbergFMSW() {}

      /// overwrite IFunction base class methods
      std::string name()const{return "TestDampedHisenbergFMSW";}

      double wrap_functionMD(const Mantid::API::IMDIterator& r) const
      { return (functionMD(r)); };
    };
    DECLARE_FUNCTION(TestDampedHisenbergFMSW)
  }
}






class DampedHisenbergFMSWTest : public CxxTest::TestSuite
{
private:
  std::string testWrkspc;
  IMDEventWorkspace_sptr inMDwrkspc;
  boost::shared_ptr<Mantid::MDAlgorithms::RunParam> rParam2;
//
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DampedHisenbergFMSWTest *createSuite() { return new DampedHisenbergFMSWTest(); }
  static void destroySuite( DampedHisenbergFMSWTest *suite ) { delete suite; }

    // create a test data set of 6 MDPoints contributing to 4 MDCells with 1, 2 and 3, 4 points each.
    DampedHisenbergFMSWTest()
    {

        testWrkspc="testMDEWrksp";
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

    }

    void testFunction()
    {
      // build a workspace with one contributing pixel
      boost::shared_ptr<Mantid::MDEvents::MDEventWorkspace<Mantid::MDEvents::MDEvent<4>,4> >
                mdSpace = MDEventsTestHelper::makeMDEWFull<4>(1,-2.0,12.0);
      // Add one point that corresponds to the centre of detector 40 in HET with ei as set in demo example
      double pos[4]={-1.728313999,0.,1.04637197,11.75};
      uint16_t runIndex=1;
      int32_t detectorId;
      float signal;
      float errorsq=1.0;
      ProgressText * prog = NULL;
      errorsq=1.0;
      std::vector<MDEvent<4> > events;
      signal=static_cast<float>(10.);
      detectorId=static_cast<int32_t>(40);
      events.push_back(MDEvent<4> (signal,errorsq,runIndex,detectorId,pos));
      // add the one event to the workspace
      mdSpace->addManyEvents(events,prog);
      // need to do this to update the signal values
      mdSpace->refreshCache();
      // check workspace
      TS_ASSERT_EQUALS(mdSpace->getNumDims(),4);
      TS_ASSERT_EQUALS(mdSpace->getNPoints(),1);
      const IMDIterator* it = mdSpace->createIterator();
      TS_ASSERT_EQUALS( it->getDataSize() ,1);
      TS_ASSERT_EQUALS( it->getNumEvents() ,1);
      // test - attempt to invoke bare function
      // Note that rParam2 data is for demo example from Tobyfit for HET instrument
      // As only CobaltSpinWaveDSHO model implemented, test data on that BUT this is
      // not the model used for h demo example.
      // Note that TF -> Mantid involves axis interchange.
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
          14., 18., 10., // sample size in Mantid axes
          1, 10., 0.5
      ));

      rParam2->setSx(0.); // disable sample shape contribution TODO debug sample shape code

      // For each detector need phi,beta,x2 and detWidth,detHeight,detDepth - values for HET detector 40 from demo example:
      // deps=0.5 for detector energy width
      rParam2->setDetInfo(40,Kernel::V3D(0.4461,0.,2.512),Kernel::V3D(0.025,0.300,0.025), 0.5);

      TestDampedHisenbergFMSW* fn = new TestDampedHisenbergFMSW();
      fn->initialize();
      // set parameters for model 111, following example case demo from TF
      fn->setParameter("Amplitude",20.,true);
      fn->setParameter("Gap",2.5,true);
      fn->setParameter("JS1",9.0,true);
      fn->setParameter("JS2",0.0,true);
      fn->setParameter("JS3",0.0,true);
      fn->setParameter("Gamma",0.5,true);
      // check default attributes for function
      int mcLoopMin = fn->getAttribute("MCLoopMin").asInt();
      TS_ASSERT_EQUALS(mcLoopMin,100)
      int mcLoopMax = fn->getAttribute("MCLoopMax").asInt();
      TS_ASSERT_EQUALS(mcLoopMax,1000)
      double mcTol = fn->getAttribute("MCTol").asDouble();
      TS_ASSERT_DELTA(mcTol,1e-5,1e-12)

      fn->setRunDataInfo(rParam2);
      fn->setRunDataInfo(rParam2);
      fn->setMagneticForm(25,3);

      double result = fn->wrap_functionMD(*it);
      TS_ASSERT_DELTA(result,0.00169,0.00003); // result from TF, 100 iterations
      fn->setAttributeValue("MCLoopMin",1000);
      fn->setAttributeValue("MCLoopMax",10000);
      result = fn->wrap_functionMD(*it);
      TS_ASSERT_DELTA(result,0.00168,0.00002); // result from TF 10000 iterations

      fn->setAttributeValue("MCLoopMin",2); // Max beats Min
      fn->setAttributeValue("MCLoopMax",1);
      result = fn->wrap_functionMD(*it);
      // check result for one Sobol iteration, where centre point is used (all perturbations zero)
      TS_ASSERT_DELTA(result,0.001903,1e-6);
      //
    }


    void testTidyUp()
    {
       removeWS(testWrkspc);
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

#endif /*DAMPEDHISENBERGTEST_H_*/

