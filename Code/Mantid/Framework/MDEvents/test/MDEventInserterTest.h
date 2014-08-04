#ifndef MANTID_MDEVENTS_MDEVENTINSERTERTEST_H_
#define MANTID_MDEVENTS_MDEVENTINSERTERTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDEvent.h"
#include <iostream>
#include <iomanip>

#include "MantidMDEvents/MDEventInserter.h"


using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Mantid::API;

class MDEventInserterTest : public CxxTest::TestSuite
{

private:

  /// Test helper method. Creates an empty 2D MDEventWorkspace, with the specified event type.
  IMDEventWorkspace_sptr createInputWorkspace(const std::string& eventType)
  {
    IAlgorithm_sptr createAlg = AlgorithmManager::Instance().createUnmanaged("CreateMDWorkspace");
    createAlg->initialize();
    createAlg->setChild(true);
    createAlg->setProperty("Dimensions", 2);
    createAlg->setPropertyValue("Extents", "-10,10,-10,10");
    createAlg->setPropertyValue("Names", "A, B");
    createAlg->setPropertyValue("Units", "m, m");
    createAlg->setPropertyValue("EventType", eventType);
    createAlg->setPropertyValue("OutputWorkspace", "out_ws");
    createAlg->execute();
    Workspace_sptr temp = createAlg->getProperty("OutputWorkspace");
    IMDEventWorkspace_sptr outWS = boost::dynamic_pointer_cast<IMDEventWorkspace>(temp);
    return outWS;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDEventInserterTest *createSuite() { return new MDEventInserterTest(); }
  static void destroySuite( MDEventInserterTest *suite ) { delete suite; }

  MDEventInserterTest()
  {
    FrameworkManager::Instance();
  }


  void test_add_md_lean_events()
  {
    typedef MDEventWorkspace<MDLeanEvent<2>, 2> MDEW_LEAN_2D; 
    
    // Check the type deduction used internally in the MDEventInserter template.
    TS_ASSERT_EQUALS(sizeof(MDEW_LEAN_2D::MDEventType), sizeof(MDEventInserter<MDEW_LEAN_2D::sptr>::MDEventType));

    // Create an input workspace
    MDEW_LEAN_2D::sptr ws2d = boost::dynamic_pointer_cast<MDEW_LEAN_2D>(createInputWorkspace("MDLeanEvent"));

    // Create the inserter.
    MDEventInserter<MDEW_LEAN_2D::sptr> inserter(ws2d);

    // Add one md event
    Mantid::coord_t coord1[2] = {-1, -1}; 
    float expectedSignal = 1;
    float expectedErrorSq = 2;
    inserter.insertMDEvent(expectedSignal, expectedErrorSq, 1, 1, coord1);
    ws2d->refreshCache();

    // Check the mdevent via the parent box.
    TS_ASSERT_EQUALS(1, ws2d->getNPoints());
    TS_ASSERT_EQUALS(expectedSignal, ws2d->getBox()->getSignal());
    TS_ASSERT_EQUALS(expectedErrorSq, ws2d->getBox()->getErrorSquared());

    // Add another md event
    inserter.insertMDEvent(expectedSignal, expectedErrorSq, 1, 1, coord1);
    ws2d->refreshCache();
    TS_ASSERT_EQUALS(2, ws2d->getNPoints());

    
  }

  void test_add_md_full_events()
  {
    typedef MDEventWorkspace<MDEvent<2>, 2> MDEW_2D; 

    // Check the type deduction used internally in the MDEventInserter template.
    TS_ASSERT_EQUALS(sizeof(MDEW_2D::MDEventType), sizeof(MDEventInserter<MDEW_2D::sptr>::MDEventType));

    // Create an input workspace.
    MDEW_2D::sptr ws2d = boost::dynamic_pointer_cast<MDEW_2D>(createInputWorkspace("MDEvent"));

    // Create the inserter.
    MDEventInserter<MDEW_2D::sptr> inserter(ws2d);

    // Add one md event
    Mantid::coord_t coord1[2] = {-1, -1}; 
    float expectedSignal = 1;
    float expectedErrorSq = 2;
    inserter.insertMDEvent(expectedSignal, expectedErrorSq, 1, 1, coord1);
    ws2d->refreshCache();

    // Check the mdevent via the parent box.
    TS_ASSERT_EQUALS(1, ws2d->getNPoints());
    TS_ASSERT_EQUALS(expectedSignal, ws2d->getBox()->getSignal());
    TS_ASSERT_EQUALS(expectedErrorSq, ws2d->getBox()->getErrorSquared());

    // Add another md event
    inserter.insertMDEvent(expectedSignal, expectedErrorSq, 1, 1, coord1);
    ws2d->refreshCache();
    TS_ASSERT_EQUALS(2, ws2d->getNPoints());
    
  }


};


#endif /* MANTID_MDEVENTS_MDEVENTINSERTERTEST_H_ */
