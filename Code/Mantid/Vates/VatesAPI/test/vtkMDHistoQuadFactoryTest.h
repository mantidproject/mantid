#ifndef VTK_MD_QUAD_FACTORY_TEST_H_
#define VTK_MD_QUAD_FACTORY_TEST_H_

#include "MantidAPI/IMDIterator.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidVatesAPI/UserDefinedThresholdRange.h"
#include "MantidVatesAPI/NoThresholdRange.h"
#include "MantidVatesAPI/vtkMDHistoQuadFactory.h"
#include "MockObjects.h"
#include <cxxtest/TestSuite.h>
#include "MantidVatesAPI/vtkStructuredGrid_Silent.h"
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
class vtkMDHistoQuadFactoryTest: public CxxTest::TestSuite
{

public:

  void testIsValidThrowsWhenNoWorkspace()
  {
    using namespace Mantid::VATES;
    using namespace Mantid::API;

    IMDWorkspace* nullWorkspace = NULL;
    Mantid::API::IMDWorkspace_sptr ws_sptr(nullWorkspace);

    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(0, 100);
    vtkMDHistoQuadFactory factory(ThresholdRange_scptr(pRange), "signal");

    TSM_ASSERT_THROWS("No workspace, so should not be possible to complete initialization.", factory.initialize(ws_sptr), std::invalid_argument);
  }

  void testCreateWithoutInitializeThrows()
  {
    FakeProgressAction progressUpdate;

    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(0, 100);
    vtkMDHistoQuadFactory factory(ThresholdRange_scptr(pRange), "signal");
    TS_ASSERT_THROWS(factory.create(progressUpdate), std::runtime_error);
  }

  void testInsideThresholds()
  {
    FakeProgressAction progressUpdate;

    // WS with 2 dimensions
    Mantid::API::IMDWorkspace_sptr ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2);

    //Thresholds have been set such that the signal values (hard-coded to 1, see above) will fall between the minimum 0 and maximum 2.
    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(0, 2);
    vtkMDHistoQuadFactory inside(ThresholdRange_scptr(pRange), "signal");
    inside.initialize(ws_sptr);
    vtkUnstructuredGrid* insideProduct = dynamic_cast<vtkUnstructuredGrid*>(inside.create(progressUpdate));

    TS_ASSERT_EQUALS((10*10), insideProduct->GetNumberOfCells());
    TS_ASSERT_EQUALS((11*11), insideProduct->GetNumberOfPoints());
  }

  void testAboveThreshold()
  {
    FakeProgressAction progressUpdate;
    // WS with 2 dimensions
    Mantid::API::IMDWorkspace_sptr ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2);

    //Thresholds have been set such that the signal values (hard-coded to 1, see above) will fall above and outside the minimum 0 and maximum 0.5.
    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(0, 0.5);
    vtkMDHistoQuadFactory above(ThresholdRange_scptr(pRange), "signal");
    above.initialize(ws_sptr);
    vtkUnstructuredGrid* aboveProduct = dynamic_cast<vtkUnstructuredGrid*>(above.create(progressUpdate));

    // This changed from previously, in order to ensure that we do not pass on empty 
    // workspaces. A single point is created in the center by the vtkNullUnstructuredGrid
    TS_ASSERT_EQUALS(1, aboveProduct->GetNumberOfCells());
    TS_ASSERT_EQUALS(1, aboveProduct->GetNumberOfPoints());
  }

  void testBelowThreshold()
  {
    FakeProgressAction progressUpdate;
    // WS with 2 dimensions
    Mantid::API::IMDWorkspace_sptr ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2);

    //Thresholds have been set such that the signal values (hard-coded to 1, see above) will fall below and outside the minimum 1.5 and maximum 2.
    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(1.5, 2);
    vtkMDHistoQuadFactory below(ThresholdRange_scptr(pRange), "signal");

    below.initialize(ws_sptr);
    vtkUnstructuredGrid* belowProduct = dynamic_cast<vtkUnstructuredGrid*>(below.create(progressUpdate));

    // This changed from previously, in order to ensure that we do not pass on empty 
    // workspaces. A single point is created in the center by the vtkNullUnstructuredGrid
    TS_ASSERT_EQUALS(1, belowProduct->GetNumberOfCells());
    TS_ASSERT_EQUALS(1, belowProduct->GetNumberOfPoints());
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
    vtkMDHistoQuadFactory factory(ThresholdRange_scptr(pRange), "signal");

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
    vtkMDHistoQuadFactory factory(ThresholdRange_scptr(pRange), "signal");

    TSM_ASSERT_THROWS("Should have thrown an execption given that no successor was available.", factory.initialize(ws_sptr), std::runtime_error);
  }

  void testCreateDelegates()
  {
    FakeProgressAction progressUpdate;
    //If the workspace provided is not a 2D imdworkspace, it should call the successor's initalization
    // WS with 1 dimension
    Mantid::API::IMDWorkspace_sptr ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 1);

    MockvtkDataSetFactory* pMockFactorySuccessor = new MockvtkDataSetFactory;
    EXPECT_CALL(*pMockFactorySuccessor, initialize(_)).Times(1); //expect it then to call initialize on the successor.
    EXPECT_CALL(*pMockFactorySuccessor, create(Ref(progressUpdate))).Times(1).WillOnce(Return(vtkStructuredGrid::New())); //expect it then to call create on the successor.
    EXPECT_CALL(*pMockFactorySuccessor, getFactoryTypeName()).WillOnce(testing::Return("TypeA")); 

    //Constructional method ensures that factory is only suitable for providing mesh information.
    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(0, 1);
    vtkMDHistoQuadFactory factory(ThresholdRange_scptr(pRange), "signal");

    //Successor is provided.
    factory.SetSuccessor(pMockFactorySuccessor);
    
    factory.initialize(ws_sptr);
    factory.create(progressUpdate); // should be called on successor.

    TSM_ASSERT("successor factory not used as expected.", Mock::VerifyAndClearExpectations(pMockFactorySuccessor));
  }

  
  void testTypeName()
  {
    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(0, 1);
    vtkMDHistoQuadFactory factory(ThresholdRange_scptr(pRange), "signal");
    TS_ASSERT_EQUALS("vtkMDHistoQuadFactory", factory.getFactoryTypeName());
  }

  void testProgressUpdates()
  {
    MockProgressAction mockProgressAction;
    //Expectation checks that progress should be >= 0 and <= 100 and called at least once!
    EXPECT_CALL(mockProgressAction, eventRaised(AllOf(Le(100),Ge(0)))).Times(AtLeast(1));

    MDHistoWorkspace_sptr ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2);
    vtkMDHistoQuadFactory factory(ThresholdRange_scptr(new NoThresholdRange), "signal");

    factory.initialize(ws_sptr);
    vtkDataSet* product= factory.create(mockProgressAction);

    TSM_ASSERT("Progress Updates not used as expected.", Mock::VerifyAndClearExpectations(&mockProgressAction));
    product->Delete();
  }

};

//=====================================================================================
// Performance tests
//=====================================================================================
class vtkMDHistoQuadFactoryTestPerformance : public CxxTest::TestSuite
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
    FakeProgressAction progressUpdate;
    //Thresholds have been set such that the signal values (hard-coded to 1, see above) will fall between the minimum 0 and maximum 2.
    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(0, 1);
    vtkMDHistoQuadFactory factory(ThresholdRange_scptr(pRange), "signal");
    factory.initialize(m_ws_sptr);
    TS_ASSERT_THROWS_NOTHING(factory.create(progressUpdate));
	}
};

#endif
