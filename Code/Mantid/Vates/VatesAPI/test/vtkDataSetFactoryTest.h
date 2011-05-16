
#ifndef VTKDATASETFACTORYTEST_H_
#define VTKDATASETFACTORYTEST_H_

#include "MantidVatesAPI/vtkDataSetFactory.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "vtkDataSet.h"
#include "vtkFloatArray.h"

class vtkDataSetFactoryTest : public CxxTest::TestSuite
{

private:

  class MockvtkDataSetFactory : public Mantid::VATES::vtkDataSetFactory 
  {
  public:
    MOCK_CONST_METHOD0(create,
      vtkDataSet*());
    MOCK_CONST_METHOD0(createMeshOnly,
      vtkDataSet*());
    MOCK_CONST_METHOD0(createScalarArray,
      vtkFloatArray*());
    MOCK_METHOD1(initialize,
      void(boost::shared_ptr<Mantid::API::IMDWorkspace>));
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

};

#endif
