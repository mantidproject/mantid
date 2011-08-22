#ifndef ESCALATINGREBINNINGACTIONMANAGERTEST_H_
#define ESCALATINGREBINNINGACTIONMANAGERTEST_H_

#include <cxxtest/TestSuite.h>
#include <vtkCharArray.h>
#include <vtkFieldData.h>
#include "MantidVatesAPI/EscalatingRebinningActionManager.h"

using namespace Mantid::VATES;

class EscalatingRebinningActionManagerTest : public CxxTest::TestSuite
{

public:
  
  void testDefaultConstruction()
  {
    EscalatingRebinningActionManager escManager;
    TSM_ASSERT_EQUALS("Wrong default level. Should be lowest escalation level.", UseCache, escManager.action());
  }

  void testConstructor()
  {
    EscalatingRebinningActionManager escManager(RecalculateAll);
    TSM_ASSERT_EQUALS("Constructor/initalized value does not reflect result of action(). Not wired-up properly.", RecalculateAll, escManager.action());
  }

  void testExpecedEscalationTypes()
  {
    //This ordering is fundamental to the operation of the EscalatingRebinningManagerTest.

    TS_ASSERT(RecalculateVisualDataSetOnly > UseCache);
    TS_ASSERT(RecalculateAll > RecalculateVisualDataSetOnly);
	  TS_ASSERT(ReloadAndRecalculateAll > RecalculateAll)
  }

  void testEscalation()
  {
    EscalatingRebinningActionManager escManager;
    RebinningActionManager& manager = escManager;

    manager.ask(RecalculateVisualDataSetOnly);
    TSM_ASSERT_EQUALS("Should have escalated to RecalculateVisualDataSetOnly", RecalculateVisualDataSetOnly, manager.action());

    manager.ask(RecalculateAll);
    TSM_ASSERT_EQUALS("Should have escalated to RecalculateAll", RecalculateAll, manager.action());
  }

  void testNoEscalation()
  {
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
    EscalatingRebinningActionManager escManager;
    RebinningActionManager& manager = escManager;

    manager.ask(RecalculateAll);
    manager.reset();

    TSM_ASSERT_EQUALS("Should have reset to lowest level.", UseCache, manager.action());
  }

};

#endif /* ESCALATINGREBINNINGACTIONMANAGERTEST_H_ */
