#ifndef NULL_REBINNING_PRESENTER_TEST_H_
#define NULL_REBINNING_PRESENTER_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidVatesAPI/NullRebinningPresenter.h"
#include "MantidVatesAPI/ProgressAction.h"

using namespace Mantid::VATES;

class NullRebinningPresenterTest : public CxxTest::TestSuite
{
private:

  class FakeProgressAction : public ProgressAction
  {
    virtual void eventRaised(double){}
  };

public:
  
  void testUpdateModelThrows()
  {
    NullRebinningPresenter nullObject;
    TS_ASSERT_THROWS(nullObject.updateModel(), std::runtime_error);
  }

  void executeThrows()
  {
    NullRebinningPresenter nullObject;
    FakeProgressAction progressAction;
    TS_ASSERT_THROWS(nullObject.execute(progressAction), std::runtime_error);
  }

  void getAppliedGeometryXMLThrows()
  {
    NullRebinningPresenter nullObject;
    TS_ASSERT_THROWS(nullObject.getAppliedGeometryXML(), std::runtime_error);
  }

};

#endif