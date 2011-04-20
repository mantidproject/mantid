#ifndef FILTERING_UPDATE_PROGRESS_ACTION_TEST
#define FILTERING_UPDATE_PROGRESS_ACTION_TEST

#include <cxxtest/TestSuite.h>
#include "MantidVatesAPI/FilteringUpdateProgressAction.h"

class FilteringUpdateProgressActionTest: public CxxTest::TestSuite
{

private:

  //Actually a fake, wouldn't make sense to mock a concrete type. Effectively the view.
  struct MockFilter
  {
    double Progress;
    void UpdateAlgorithmProgress(double progress)
    {
      this->Progress = progress;
    }
  };


  typedef Mantid::VATES::FilterUpdateProgressAction<MockFilter> ProgressActionType;

public:

  void testCallsView()
  {
    //Test that is wired up correctly.
    MockFilter view;
    ProgressActionType  model(&view);
    model.eventRaised(10);
    TSM_ASSERT_EQUALS("View and model are not wired up correctly for progress updating.", 10, view.Progress);
  }

  void testIsProgressAction()
  {
    //Test that template works through abstract data type.
    MockFilter view;
    ProgressActionType  model(&view);
    Mantid::VATES::ProgressAction& imodel = model;
    imodel.eventRaised(10);
    TSM_ASSERT_EQUALS("View and model are not wired up correctly for progress updating.", 10, view.Progress);
  }

};
#endif