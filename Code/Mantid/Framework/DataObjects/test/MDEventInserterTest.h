#ifndef MANTID_DATAOBJECTS_MDEVENTINSERTERTEST_H_
#define MANTID_DATAOBJECTS_MDEVENTINSERTERTEST_H_

#include "MantidKernel/Timer.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/MDEvent.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDEventInserter.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"

#include <boost/shared_ptr.hpp>

#include <cxxtest/TestSuite.h>


using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::API;

class MDEventInserterTest : public CxxTest::TestSuite
{

private:

  /// Test helper method. Creates an empty 2D MDEventWorkspace, with the specified event type.
  IMDEventWorkspace_sptr createInputWorkspace(const std::string& eventType)
  {
    using Mantid::Geometry::MDHistoDimension;

    IMDEventWorkspace_sptr ws =
        MDEventFactory::CreateMDWorkspace(2, eventType);
    coord_t min(-10.0f), max(10.0f);
    ws->addDimension(boost::make_shared<MDHistoDimension>("A", "A", "m", min, max, 1));
    ws->addDimension(boost::make_shared<MDHistoDimension>("B", "B", "m", min, max, 1));
    ws->initialize();
    // Split to level 1
    ws->splitBox();
    ws->setMinRecursionDepth(0);
    return ws;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDEventInserterTest *createSuite() { return new MDEventInserterTest(); }
  static void destroySuite( MDEventInserterTest *suite ) { delete suite; }

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


#endif /* MANTID_DATAOBJECTS_MDEVENTINSERTERTEST_H_ */
