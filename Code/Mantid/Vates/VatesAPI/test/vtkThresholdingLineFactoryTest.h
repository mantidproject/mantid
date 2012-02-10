#ifndef VTK_THRESHOLDING_LINE_FACTORY_TEST_H
#define VTK_THRESHOLDING_LINE_FACTORY_TEST_H

#include "MantidAPI/IMDIterator.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidVatesAPI/UserDefinedThresholdRange.h"
#include "MantidVatesAPI/vtkThresholdingLineFactory.h"
#include "MockObjects.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::VATES;
using namespace testing;


//=====================================================================================
// Functional tests
//=====================================================================================
class vtkThresholdingLineFactoryTest: public CxxTest::TestSuite
{

public:

  void testIsValidThrowsWhenNoWorkspace()
  {
    IMDWorkspace* nullWorkspace = NULL;
    Mantid::API::IMDWorkspace_sptr ws_sptr(nullWorkspace);

    vtkThresholdingLineFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 10000)), "signal");

    TSM_ASSERT_THROWS("No workspace, so should not be possible to complete initialization.", factory.initialize(ws_sptr), std::runtime_error);
  }

  void testCreateWithoutInitializeThrows()
  {
    vtkThresholdingLineFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 10000)), "signal");
    TS_ASSERT_THROWS(factory.create(), std::runtime_error);
  }

  void testInsideThresholds()
  {
    Mantid::API::IMDWorkspace_sptr ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 1);

    //Thresholds have been set such that the signal values (hard-coded to 1, see above) will fall between the minimum 0 and maximum 2.
    vtkThresholdingLineFactory inside(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 2)), "signal");
    inside.initialize(ws_sptr);
    vtkUnstructuredGrid* insideProduct = dynamic_cast<vtkUnstructuredGrid*>(inside.create());

    TS_ASSERT_EQUALS(9, insideProduct->GetNumberOfCells());
    TS_ASSERT_EQUALS(10, insideProduct->GetNumberOfPoints());
  }

  void testAboveThreshold()
  {
    using namespace Mantid::Geometry;
    using namespace testing;

    Mantid::API::IMDWorkspace_sptr ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 1);

    //Thresholds have been set such that the signal values (hard-coded to 1, see above) will fall above and outside the minimum 0 and maximum 0.5.
    vtkThresholdingLineFactory above(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 0.5)), "signal");
    above.initialize(ws_sptr);
    vtkUnstructuredGrid* aboveProduct = dynamic_cast<vtkUnstructuredGrid*>(above.create());

    TS_ASSERT_EQUALS(0, aboveProduct->GetNumberOfCells());
    TS_ASSERT_EQUALS(10, aboveProduct->GetNumberOfPoints());
  }

  void testBelowThreshold()
  {
    Mantid::API::IMDWorkspace_sptr ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 1);

    //Thresholds have been set such that the signal values (hard-coded to 1, see above) will fall below and outside the minimum 1.5 and maximum 2.
    vtkThresholdingLineFactory below(ThresholdRange_scptr(new UserDefinedThresholdRange(1.5, 2)), "signal");
    below.initialize(ws_sptr);
    vtkUnstructuredGrid* belowProduct = dynamic_cast<vtkUnstructuredGrid*>(below.create());

    TS_ASSERT_EQUALS(0, belowProduct->GetNumberOfCells());
    TS_ASSERT_EQUALS(10, belowProduct->GetNumberOfPoints());
  }

  void testInitializationDelegates()
  {
    //If the workspace provided is not a 1D imdworkspace, it should call the successor's initalization
    // 3 dimensions on the workspace
    Mantid::API::IMDWorkspace_sptr ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 3);

    MockvtkDataSetFactory* pMockFactorySuccessor = new MockvtkDataSetFactory;
    EXPECT_CALL(*pMockFactorySuccessor, initialize(_)).Times(1); //expect it then to call initialize on the successor.
    EXPECT_CALL(*pMockFactorySuccessor, getFactoryTypeName()).WillOnce(testing::Return("TypeA")); 

    //Constructional method ensures that factory is only suitable for providing mesh information.
    vtkThresholdingLineFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 10000)), "signal");

    //Successor is provided.
    factory.SetSuccessor(pMockFactorySuccessor);
    factory.initialize(ws_sptr);

    TSM_ASSERT("successor factory not used as expected.", Mock::VerifyAndClearExpectations(pMockFactorySuccessor));
  }

  void testInitializationDelegatesThrows()
  {
    //If the workspace provided is not a 2D imdworkspace, it should call the successor's initalization. If there is no successor an exception should be thrown.
    // 3 dimensions on the workspace
    Mantid::API::IMDWorkspace_sptr ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 3);

    //Constructional method ensures that factory is only suitable for providing mesh information.
    vtkThresholdingLineFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 10000)),"signal");

    TSM_ASSERT_THROWS("Should have thrown an execption given that no successor was available.", factory.initialize(ws_sptr), std::runtime_error);
  }

  void testCreateDelegates()
  {
    //If the workspace provided is not a 2D imdworkspace, it should call the successor's initalization
    // 3 dimensions on the workspace
    Mantid::API::IMDWorkspace_sptr ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 3);

    MockvtkDataSetFactory* pMockFactorySuccessor = new MockvtkDataSetFactory;
    EXPECT_CALL(*pMockFactorySuccessor, initialize(_)).Times(1); //expect it then to call initialize on the successor.
    EXPECT_CALL(*pMockFactorySuccessor, create()).Times(1); //expect it then to call create on the successor.
    EXPECT_CALL(*pMockFactorySuccessor, getFactoryTypeName()).WillOnce(testing::Return("TypeA")); 

    //Constructional method ensures that factory is only suitable for providing mesh information.
    vtkThresholdingLineFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 10000)),"signal");

    //Successor is provided.
    factory.SetSuccessor(pMockFactorySuccessor);
    
    factory.initialize(ws_sptr);
    factory.create(); // should be called on successor.

    TSM_ASSERT("successor factory not used as expected.", Mock::VerifyAndClearExpectations(pMockFactorySuccessor));
  }

  void testTypeName()
  {
    vtkThresholdingLineFactory factory (ThresholdRange_scptr(new UserDefinedThresholdRange(0, 10000)),"signal");
    TS_ASSERT_EQUALS("vtkThresholdingLineFactory", factory.getFactoryTypeName());
  }

};

//=====================================================================================
// Performance tests
//=====================================================================================
class vtkThresholdingLineFactoryTestPerformance : public CxxTest::TestSuite
{

private:
   Mantid::API::IMDWorkspace_sptr m_ws_sptr;
public:

  void setUp()
  {
    //1D Workspace with 2000 points
    m_ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 1, 200000);
  }

	void testGenerateVTKDataSet()
	{
    //Thresholds have been set such that the signal values (hard-coded to 1, see above) will fall between the minimum 0 and maximum 2.
    vtkThresholdingLineFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 2)),"signal");
    factory.initialize(m_ws_sptr);
    TS_ASSERT_THROWS_NOTHING(factory.create());
	}

};

#endif
