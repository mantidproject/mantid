#ifndef NULL_REBINNING_PRESENTER_TEST_H_
#define NULL_REBINNING_PRESENTER_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidVatesAPI/NullRebinningPresenter.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/vtkDataSetFactory.h"
#include <vtkDataSet.h>
#include <vtkFloatArray.h>
#include "MantidAPI/Workspace.h"

using namespace Mantid::VATES;

class NullRebinningPresenterTest : public CxxTest::TestSuite
{
private:

  class FakeProgressAction : public ProgressAction
  {
    virtual void eventRaised(double){}
  };

  class FakeDataSetFactory : public Mantid::VATES::vtkDataSetFactory 
  {
  public:
    virtual ~FakeDataSetFactory(){ }
  private:
    
    virtual vtkDataSet* create(ProgressAction&) const{ throw std::runtime_error("Not implemented on test type");}
    virtual void initialize(boost::shared_ptr<Mantid::API::Workspace>){throw std::runtime_error("Not implemented on test type");}
    virtual void SetSuccessor(vtkDataSetFactory*){ throw std::runtime_error("Not implemented on test type");}
    virtual bool hasSuccessor() const{ throw std::runtime_error("Not implemented on test type");}
    virtual std::string getFactoryTypeName() const{ throw std::runtime_error("Not implemented on test type");}
    virtual void validate() const{ throw std::runtime_error("Not implemented on test type");}
  };

public:
  
  void testUpdateModelDoewNothing()
  {
    NullRebinningPresenter nullObject;
    TS_ASSERT_THROWS_NOTHING(nullObject.updateModel());
  }

  void executeThrows()
  {
    NullRebinningPresenter nullObject;
    FakeProgressAction progressAction;
    FakeDataSetFactory* pFactory = new FakeDataSetFactory;
    TS_ASSERT_THROWS(nullObject.execute(pFactory, progressAction, progressAction), std::runtime_error);
    delete pFactory;
  }

  void getAppliedGeometryXMLThrows()
  {
    NullRebinningPresenter nullObject;
    TS_ASSERT_THROWS(nullObject.getAppliedGeometryXML(), std::runtime_error);
  }

  void hasTDimensionAvailableThrows()
  {
    NullRebinningPresenter nullObject;
    TS_ASSERT_THROWS(nullObject.hasTDimensionAvailable(), std::runtime_error);
  }

  void getTimeStepValuesThrows()
  {
    NullRebinningPresenter nullObject;
    TS_ASSERT_THROWS(nullObject.getTimeStepValues(), std::runtime_error);
  }

};

#endif
