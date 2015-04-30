#ifndef MANTID_MDALGORITHMS_CONVERTTODETECTORFACEMDTEST_H_
#define MANTID_MDALGORITHMS_CONVERTTODETECTORFACEMDTEST_H_

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Events.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidMDAlgorithms/ConvertToDetectorFaceMD.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using Mantid::Geometry::IMDDimension_const_sptr;

class ConvertToDetectorFaceMDTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertToDetectorFaceMDTest *createSuite() { return new ConvertToDetectorFaceMDTest(); }
  static void destroySuite( ConvertToDetectorFaceMDTest *suite ) { delete suite; }


  void test_Init()
  {
    ConvertToDetectorFaceMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  //----------------------------------------------------------------------------
  EventWorkspace_sptr makeTestWS(EventType type)
  {
    EventWorkspace_sptr in_ws = WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(5, 10, false);
    if ((type == WEIGHTED) || (type == WEIGHTED_NOTIME))
    {
      for (size_t i =0; i<in_ws->getNumberHistograms(); i++)
      {
        EventList & el = in_ws->getEventList(i);
        if (type == WEIGHTED)
          el.multiply(2.0);
        else
          el.compressEvents(0.0, &el);
      }
    }
    return in_ws;
  }


  //----------------------------------------------------------------------------
  template <class WSTYPE>
  boost::shared_ptr<WSTYPE> doTest(EventType type, const std::string & BankNumbers)
  {
    EventWorkspace_sptr in_ws = makeTestWS(type);
    ConvertToDetectorFaceMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    alg.setProperty("InputWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(in_ws) );
    alg.setPropertyValue("BankNumbers", BankNumbers);
    alg.setPropertyValue("OutputWorkspace", "output_md");
    TS_ASSERT_THROWS_NOTHING( alg.execute(); )
    TS_ASSERT( alg.isExecuted() )

    boost::shared_ptr<WSTYPE> ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<WSTYPE>("output_md") );
    TS_ASSERT(ws);
    if (!ws) return ws;

    TS_ASSERT_EQUALS(ws->getNumExperimentInfo(), 1);

    for (size_t d=0; d<2; d++)
    {
      IMDDimension_const_sptr dim = ws->getDimension(d);
      TS_ASSERT_EQUALS( dim->getName(), (d == 0 ? "x" : "y") );
      TS_ASSERT_EQUALS( dim->getNBins(), 10);
      TS_ASSERT_DELTA(  dim->getMinimum(), 0, 1e-5);
      TS_ASSERT_DELTA(  dim->getMaximum(),10, 1e-5);
      TS_ASSERT_EQUALS( dim->getUnits(), "pixel");
    }
    IMDDimension_const_sptr dim = ws->getDimension(2);
    TS_ASSERT_EQUALS( dim->getName(), "dSpacing" );
    TS_ASSERT_EQUALS( dim->getNBins(), 101);
    TS_ASSERT_DELTA(  dim->getMinimum(),   0, 1e-5);
    TS_ASSERT_DELTA(  dim->getMaximum(), 100, 1e-5);
    TS_ASSERT_EQUALS( dim->getUnits(), "Angstrom");

    return ws;
  }


  //----------------------------------------------------------------------------
  /** Run algorithm and check that it fails */
  void doTestFails(const std::string & BankNumbers)
  {
    EventWorkspace_sptr in_ws = makeTestWS(TOF);
    ConvertToDetectorFaceMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    alg.setProperty("InputWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(in_ws) );
    alg.setPropertyValue("BankNumbers", BankNumbers);
    alg.setPropertyValue("OutputWorkspace", "output_md");
    TS_ASSERT_THROWS_NOTHING( alg.execute(); )
    TS_ASSERT( !alg.isExecuted() )
  }

  void test_name()
  {
    ConvertToDetectorFaceMD alg;
    TS_ASSERT_EQUALS("ConvertToDetectorFaceMD", alg.name())
  }

  void test_categories()
  {
    ConvertToDetectorFaceMD alg;
    TS_ASSERT_EQUALS("MDAlgorithms", alg.category());
  }

  //----------------------------------------------------------------------------
  void test_oneBank()
  {
    MDEventWorkspace3::sptr ws = doTest<MDEventWorkspace3>(TOF, "1");
    TS_ASSERT_EQUALS( ws->getNPoints(), 20000);
  }

  void test_WeightedEvent()
  {
    MDEventWorkspace3::sptr ws3 = doTest<MDEventWorkspace3>(WEIGHTED, "1");
    TS_ASSERT_EQUALS( ws3->getNPoints(), 20000);
    MDEventWorkspace4::sptr ws4 = doTest<MDEventWorkspace4>(WEIGHTED, "1,2");
    TS_ASSERT_EQUALS( ws4->getNPoints(), 20000*2);
  }
  void test_WeightedEventNoTime()
  {
    MDEventWorkspace3::sptr ws3 = doTest<MDEventWorkspace3>(WEIGHTED_NOTIME, "1");
    TS_ASSERT_EQUALS( ws3->getNPoints(), 10000);
    MDEventWorkspace4::sptr ws4 = doTest<MDEventWorkspace4>(WEIGHTED_NOTIME, "1,2");
    TS_ASSERT_EQUALS( ws4->getNPoints(), 10000*2);
  }

  void test_nonexistentBank_fails()
  {
    doTestFails("7");
    doTestFails("0");
  }
  
  //----------------------------------------------------------------------------
  void test_severalBanks()
  {
    MDEventWorkspace4::sptr ws = doTest<MDEventWorkspace4>(TOF, "1, 3");
    TS_ASSERT_EQUALS( ws->getNPoints(), 2*20000);
    IMDDimension_const_sptr dim = ws->getDimension(3);
    TS_ASSERT_EQUALS( dim->getName(), "bank" );
    TS_ASSERT_EQUALS( dim->getNBins(), 3);
    TS_ASSERT_DELTA(  dim->getMinimum(), 1, 1e-5);
    TS_ASSERT_DELTA(  dim->getMaximum(), 4, 1e-5);
    TS_ASSERT_EQUALS( dim->getUnits(), "number");
  }

  /** If you do not specify a list of banks, all are used */
  void test_allBanks()
  {
    MDEventWorkspace4::sptr ws = doTest<MDEventWorkspace4>(TOF, "");
    TS_ASSERT_EQUALS( ws->getNPoints(), 5*20000);
    IMDDimension_const_sptr dim = ws->getDimension(3);
    TS_ASSERT_EQUALS( dim->getName(), "bank" );
    TS_ASSERT_EQUALS( dim->getNBins(), 5);
    TS_ASSERT_DELTA(  dim->getMinimum(), 1, 1e-5);
    TS_ASSERT_DELTA(  dim->getMaximum(), 6, 1e-5);
    TS_ASSERT_EQUALS( dim->getUnits(), "number");
  }


};


#endif /* MANTID_MDALGORITHMS_CONVERTTODETECTORFACEMDTEST_H_ */
