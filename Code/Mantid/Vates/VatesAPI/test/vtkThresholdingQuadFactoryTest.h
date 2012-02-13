#ifndef VTK_THRESHOLDING_QUAD_FACTORY_TEST_H
#define VTK_THRESHOLDING_QUAD_FACTORY_TEST_H

#include "MantidAPI/IMDIterator.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidVatesAPI/UserDefinedThresholdRange.h"
#include "MantidVatesAPI/vtkThresholdingQuadFactory.h"
#include "MockObjects.h"
#include <cxxtest/TestSuite.h>
#include <vtkStructuredGrid.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::VATES;
using namespace testing;
using Mantid::MDEvents::MDEventsTestHelper::makeFakeMDHistoWorkspace;


//=====================================================================================
// Functional tests
//=====================================================================================
class vtkThresholdingQuadFactoryTest: public CxxTest::TestSuite
{

public:

  void testIsValidThrowsWhenNoWorkspace()
  {
    using namespace Mantid::VATES;
    using namespace Mantid::API;

    IMDWorkspace* nullWorkspace = NULL;
    Mantid::API::IMDWorkspace_sptr ws_sptr(nullWorkspace);

    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(0, 100);
    vtkThresholdingQuadFactory factory(ThresholdRange_scptr(pRange), "signal");

    TSM_ASSERT_THROWS("No workspace, so should not be possible to complete initialization.", factory.initialize(ws_sptr), std::invalid_argument);
  }

  void testCreateWithoutInitializeThrows()
  {
    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(0, 100);
    vtkThresholdingQuadFactory factory(ThresholdRange_scptr(pRange), "signal");
    TS_ASSERT_THROWS(factory.create(), std::runtime_error);
  }

  void testInsideThresholds()
  {
    // WS with 2 dimensions
    Mantid::API::IMDWorkspace_sptr ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2);

    //Thresholds have been set such that the signal values (hard-coded to 1, see above) will fall between the minimum 0 and maximum 2.
    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(0, 2);
    vtkThresholdingQuadFactory inside(ThresholdRange_scptr(pRange), "signal");
    inside.initialize(ws_sptr);
    vtkUnstructuredGrid* insideProduct = dynamic_cast<vtkUnstructuredGrid*>(inside.create());

    TS_ASSERT_EQUALS((10*10), insideProduct->GetNumberOfCells());
    TS_ASSERT_EQUALS((11*11), insideProduct->GetNumberOfPoints());
  }

  void testAboveThreshold()
  {
    // WS with 2 dimensions
    Mantid::API::IMDWorkspace_sptr ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2);

    //Thresholds have been set such that the signal values (hard-coded to 1, see above) will fall above and outside the minimum 0 and maximum 0.5.
    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(0, 0.5);
    vtkThresholdingQuadFactory above(ThresholdRange_scptr(pRange), "signal");
    above.initialize(ws_sptr);
    vtkUnstructuredGrid* aboveProduct = dynamic_cast<vtkUnstructuredGrid*>(above.create());

    // No points nor cells are created if nothing is within range
    TS_ASSERT_EQUALS(0, aboveProduct->GetNumberOfCells());
    TS_ASSERT_EQUALS(0, aboveProduct->GetNumberOfPoints());
  }

  void testBelowThreshold()
  {
    // WS with 2 dimensions
    Mantid::API::IMDWorkspace_sptr ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2);

    //Thresholds have been set such that the signal values (hard-coded to 1, see above) will fall below and outside the minimum 1.5 and maximum 2.
    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(1.5, 2);
    vtkThresholdingQuadFactory below(ThresholdRange_scptr(pRange), "signal");

    below.initialize(ws_sptr);
    vtkUnstructuredGrid* belowProduct = dynamic_cast<vtkUnstructuredGrid*>(below.create());

    // No points nor cells are created if nothing is within range
    TS_ASSERT_EQUALS(0, belowProduct->GetNumberOfCells());
    TS_ASSERT_EQUALS(0, belowProduct->GetNumberOfPoints());
  }

  void testInitializationDelegates()
  {
    //If the workspace provided is not a 4D imdworkspace, it should call the successor's initalization
    // WS with 1 dimension
    Mantid::API::IMDWorkspace_sptr ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 1);

    MockvtkDataSetFactory* pMockFactorySuccessor = new MockvtkDataSetFactory;
    EXPECT_CALL(*pMockFactorySuccessor, getFactoryTypeName()).WillOnce(testing::Return("TypeA")); 
    EXPECT_CALL(*pMockFactorySuccessor, initialize(_)).Times(1); //expect it then to call initialize on the successor.

    //Constructional method ensures that factory is only suitable for providing mesh information.
    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(0, 1);
    vtkThresholdingQuadFactory factory(ThresholdRange_scptr(pRange), "signal");

    //Successor is provided.
    factory.SetSuccessor(pMockFactorySuccessor);
    
    factory.initialize(ws_sptr);

    TSM_ASSERT("successor factory not used as expected.", Mock::VerifyAndClearExpectations(pMockFactorySuccessor));
  }

  void testInitializationDelegatesThrows()
  {
    //If the workspace provided is not a 2D imdworkspace, it should call the successor's initalization. If there is no successor an exception should be thrown.
    // WS with 1 dimension
    Mantid::API::IMDWorkspace_sptr ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 1);

    //Constructional method ensures that factory is only suitable for providing mesh information.
    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(0, 1);
    vtkThresholdingQuadFactory factory(ThresholdRange_scptr(pRange), "signal");

    TSM_ASSERT_THROWS("Should have thrown an execption given that no successor was available.", factory.initialize(ws_sptr), std::runtime_error);
  }

  void testCreateDelegates()
  {
    //If the workspace provided is not a 2D imdworkspace, it should call the successor's initalization
    // WS with 1 dimension
    Mantid::API::IMDWorkspace_sptr ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 1);

    MockvtkDataSetFactory* pMockFactorySuccessor = new MockvtkDataSetFactory;
    EXPECT_CALL(*pMockFactorySuccessor, initialize(_)).Times(1); //expect it then to call initialize on the successor.
    EXPECT_CALL(*pMockFactorySuccessor, create()).Times(1).WillOnce(Return(vtkStructuredGrid::New())); //expect it then to call create on the successor.
    EXPECT_CALL(*pMockFactorySuccessor, getFactoryTypeName()).WillOnce(testing::Return("TypeA")); 

    //Constructional method ensures that factory is only suitable for providing mesh information.
    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(0, 1);
    vtkThresholdingQuadFactory factory(ThresholdRange_scptr(pRange), "signal");

    //Successor is provided.
    factory.SetSuccessor(pMockFactorySuccessor);
    
    factory.initialize(ws_sptr);
    factory.create(); // should be called on successor.

    TSM_ASSERT("successor factory not used as expected.", Mock::VerifyAndClearExpectations(pMockFactorySuccessor));
  }

  
  void testTypeName()
  {
    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(0, 1);
    vtkThresholdingQuadFactory factory(ThresholdRange_scptr(pRange), "signal");
    TS_ASSERT_EQUALS("vtkThresholdingQuadFactory", factory.getFactoryTypeName());
  }

};

//=====================================================================================
// Performance tests
//=====================================================================================
class vtkThresholdingQuadFactoryTestPerformance : public CxxTest::TestSuite
{
private:
  Mantid::API::IMDWorkspace_sptr m_ws_sptr;

public:

  void setUp()
  {
    // WS with 2 dimension, 100x100
    m_ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2, 1000);
    m_ws_sptr->setTransformFromOriginal(new NullCoordTransform);
  }

	void testGenerateVTKDataSet()
	{
    //Thresholds have been set such that the signal values (hard-coded to 1, see above) will fall between the minimum 0 and maximum 2.
    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(0, 1);
    vtkThresholdingQuadFactory factory(ThresholdRange_scptr(pRange), "signal");
    factory.initialize(m_ws_sptr);
    TS_ASSERT_THROWS_NOTHING(factory.create());
	}
};

#endif
