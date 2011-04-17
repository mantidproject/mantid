#ifndef ESCALATINGREBINNINGACTIONMANAGERTEST_H_
#define ESCALATINGREBINNINGACTIONMANAGERTEST_H_

#include <cxxtest/TestSuite.h>
#include <vtkCharArray.h>
#include <vtkFieldData.h>
#include "MantidVatesAPI/EscalatingRebinningActionManager.h"

class EscalatingRebinningActionManagerTest : public CxxTest::TestSuite
{

public:

  void testExpecedEscalationTypes()
  {
    //This ordering is fundamental to the operation of the EscalatingRebinningManagerTest.
    using namespace Mantid::VATES;
    TS_ASSERT(RecalculateAll > RecalculateVisualDataSetOnly);
    TS_ASSERT(RecalculateAll > UseCache);
  }

  void testEscalation()
  {
    using namespace Mantid::VATES;
    EscalatingRebinningActionManager escManager;
    RebinningActionManager& manager = escManager;

    manager.ask(RecalculateVisualDataSetOnly);
    TSM_ASSERT_EQUALS("Should have escalated to RecalculateVisualDataSetOnly", RecalculateVisualDataSetOnly, manager.action());

    manager.ask(RecalculateAll);
    TSM_ASSERT_EQUALS("Should have escalated to RecalculateAll", RecalculateAll, manager.action());
  }

  void testNoEscalation()
  {
    using namespace Mantid::VATES;
    EscalatingRebinningActionManager escManager;
    RebinningActionManager& manager = escManager;
    manager.ask(RecalculateAll);

    manager.ask(RecalculateVisualDataSetOnly);
    TSM_ASSERT_EQUALS("Should not have de-escalated to RecalculateVisualDataSetOnly", RecalculateAll, manager.action());

    manager.ask(RecalculateAll);
    TSM_ASSERT_EQUALS("Should have de-escalated to UseCache", RecalculateAll, manager.action());

  }

  void testReset()
  {
    using namespace Mantid::VATES;
    EscalatingRebinningActionManager escManager;
    RebinningActionManager& manager = escManager;

    manager.ask(RecalculateAll);
    manager.reset();

    TSM_ASSERT_EQUALS("Should have reset to lowest level.", UseCache, manager.action());
  }

};

#endif /* ESCALATINGREBINNINGACTIONMANAGERTEST_H_ */
