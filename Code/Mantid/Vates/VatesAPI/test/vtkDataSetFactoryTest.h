
#ifndef VTKDATASETFACTORYTEST_H_
#define VTKDATASETFACTORYTEST_H_

#include "MantidVatesAPI/vtkDataSetFactory.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include <vtkStructuredGrid.h>

using namespace Mantid::API;
using namespace Mantid::MDEvents;
using namespace testing;

class vtkDataSetFactoryTest : public CxxTest::TestSuite
{
private:
  class MockvtkDataSetFactory : public Mantid::VATES::vtkDataSetFactory
  {
  public:
    MOCK_CONST_METHOD0(create,
      vtkDataSet*());
    MOCK_METHOD1(initialize,
      void(boost::shared_ptr<Mantid::API::Workspace>));
    MOCK_CONST_METHOD0(validate,
      void());
    MOCK_CONST_METHOD0(getFactoryTypeName, std::string());
    void SetSuccessorConcrete(vtkDataSetFactory* pSuccessor)
    {
      return vtkDataSetFactory::SetSuccessor(pSuccessor);
    }
    bool hasSuccessorConcrete() const
    {
      return vtkDataSetFactory::hasSuccessor();
    }
  };

public:

  void testSetSuccessor()
  {
    MockvtkDataSetFactory factory;
    MockvtkDataSetFactory* pSuccessor = new MockvtkDataSetFactory;
    
    EXPECT_CALL(factory, getFactoryTypeName()).WillOnce(testing::Return("TypeA")); 
    EXPECT_CALL(*pSuccessor, getFactoryTypeName()).WillOnce(testing::Return("TypeB")); //Different type name, so setting the successor should work.
    factory.SetSuccessor(pSuccessor);

    TSM_ASSERT("Successor should have been set", factory.hasSuccessor());
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(&factory));
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(pSuccessor));
  }

  void testSetSuccessorThrows()
  {
    MockvtkDataSetFactory factory;
    MockvtkDataSetFactory* pSuccessor = new MockvtkDataSetFactory;
    EXPECT_CALL(factory, getFactoryTypeName()).WillOnce(testing::Return("TypeA")); 
    EXPECT_CALL(*pSuccessor, getFactoryTypeName()).WillOnce(testing::Return("TypeA")); //Same type name. should NOT work.
    TSM_ASSERT_THROWS("By default, should throw when successor type is the same as the container.", factory.SetSuccessor(pSuccessor), std::runtime_error);
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(&factory));
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(pSuccessor));
  }

  void testEnumValues()
  {
    using Mantid::VATES::vtkDataSetFactory;
    int oneD = vtkDataSetFactory::OneDimensional;
    int twoD = vtkDataSetFactory::TwoDimensional;
    int threeD = vtkDataSetFactory::ThreeDimensional;
    int fourD = vtkDataSetFactory::FourDimensional;
    TS_ASSERT_EQUALS(1, oneD);
    TS_ASSERT_EQUALS(2, twoD);
    TS_ASSERT_EQUALS(3, threeD);
    TS_ASSERT_EQUALS(4, fourD);
  }

  void testCheckDimensionalityByDefault()
  {
    MockvtkDataSetFactory factory;
    TS_ASSERT(factory.doesCheckDimensionality());
  }

  void testSetCheckDimensionality()
  {
    MockvtkDataSetFactory factory;
    factory.setCheckDimensionality(false);
    TS_ASSERT(!factory.doesCheckDimensionality());
    factory.setCheckDimensionality(true);
    TS_ASSERT(factory.doesCheckDimensionality());
  }

  void testOneStepCreate()
  {
    MockvtkDataSetFactory factory;
    EXPECT_CALL(factory, initialize(_)).Times(1);
    EXPECT_CALL(factory, create()).Times(1).WillOnce(Return(vtkStructuredGrid::New()));

    IMDHistoWorkspace_sptr ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2);
    vtkDataSet* product = factory.oneStepCreate(ws_sptr);
    TS_ASSERT(product != NULL);
    TSM_ASSERT_EQUALS("Output not wired up correctly to ::create() method", "vtkStructuredGrid", std::string(product->GetClassName()));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&factory));
  }


};

#endif
