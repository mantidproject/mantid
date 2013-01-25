#ifndef MANTID_MDEVENTS_MDBOX_FLATTREE_H_
#define MANTID_MDEVENTS_MDBOX_FLATTREE_H_

#include "MantidAPI/FrameworkManager.h"
#include "MantidMDEvents/MDBoxFlatTree.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidMDEvents/MDLeanEvent.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::MDEvents;

class MDEventFlatTreeTest :public CxxTest::TestSuite
{
private:
  
    Mantid::MDEvents::MDEventWorkspace3Lean::sptr spEw3;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDEventFlatTreeTest *createSuite() { return new MDEventFlatTreeTest(); }
  static void destroySuite( MDEventFlatTreeTest *suite ) { delete suite; }

  void testInit()
  {
    MDBoxFlatTree BoxTree;

    TS_ASSERT_EQUALS(0,BoxTree.getNBoxes());

    TS_ASSERT_THROWS_NOTHING(BoxTree.initTree(spEw3));

    TSM_ASSERT_EQUALS("Workspace creatrion helper should generate ws split into 1001 boxes",1001,BoxTree.getNBoxes());

  }

  MDEventFlatTreeTest()
  {
      // load dependent DLL, which are used in MDEventsTestHelper (e.g. MDAlgorithms to create MD workspace)
//      Mantid::API::FrameworkManager::Instance();
    // make non-file backet mdEv workspace with 10000 events
     spEw3 = MDEventsTestHelper::makeFileBackedMDEW("TestLeanEvWS", false,10000);
  }

};


#endif