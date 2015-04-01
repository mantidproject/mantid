#ifndef VTK_MD_HISTO_HEX_FACTORY_TEST_H_
#define VTK_MD_HISTO_HEX_FACTORY_TEST_H_

#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidVatesAPI/UserDefinedThresholdRange.h"
#include "MantidVatesAPI/NoThresholdRange.h"
#include "MantidVatesAPI/vtkMDHistoHexFactory.h"
#include "MockObjects.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "MantidVatesAPI/vtkStructuredGrid_Silent.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::VATES;
using namespace Mantid::Geometry;
using namespace testing;

//=====================================================================================
// Functional Tests
//=====================================================================================
class vtkMDHistoHexFactoryTest: public CxxTest::TestSuite
{

  public:

  void testThresholds()
  {
    FakeProgressAction progressUpdate;

    // Workspace with value 1.0 everywhere
    MDHistoWorkspace_sptr ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 3);
    ws_sptr->setTransformFromOriginal(new NullCoordTransform);

    vtkMDHistoHexFactory inside(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 2)), "signal");
    inside.initialize(ws_sptr);
    vtkUnstructuredGrid* insideProduct = dynamic_cast<vtkUnstructuredGrid*>(inside.create(progressUpdate));

    vtkMDHistoHexFactory below(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 0.5)), "signal");
    below.initialize(ws_sptr);
    vtkUnstructuredGrid* belowProduct = dynamic_cast<vtkUnstructuredGrid*>(below.create(progressUpdate));

    vtkMDHistoHexFactory above(ThresholdRange_scptr(new UserDefinedThresholdRange(2, 3)), "signal");
    above.initialize(ws_sptr);
    vtkUnstructuredGrid* aboveProduct = dynamic_cast<vtkUnstructuredGrid*>(above.create(progressUpdate));

    TS_ASSERT_EQUALS((10*10*10), insideProduct->GetNumberOfCells());

    // This has changed, in order to ensure that we do not pass on empty 
    // workspaces. A single point is created in the center by the vtkNullUnstructuredGrid
    TS_ASSERT_EQUALS(1, belowProduct->GetNumberOfCells());
    TS_ASSERT_EQUALS(1, aboveProduct->GetNumberOfCells());
  }

  void testSignalAspects()
  {
    FakeProgressAction progressUpdate;

    // Workspace with value 1.0 everywhere
    MDHistoWorkspace_sptr ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 3);
    ws_sptr->setTransformFromOriginal(new NullCoordTransform);

    //Constructional method ensures that factory is only suitable for providing mesh information.
    vtkMDHistoHexFactory factory (ThresholdRange_scptr(new UserDefinedThresholdRange(0, 10000)), "signal");
    factory.initialize(ws_sptr);

    vtkDataSet* product = factory.create(progressUpdate);
    TSM_ASSERT_EQUALS("A single array should be present on the product dataset.", 1, product->GetCellData()->GetNumberOfArrays());
    vtkDataArray* signalData = product->GetCellData()->GetArray(0);
    TSM_ASSERT_EQUALS("The obtained cell data has the wrong name.", std::string("signal"), signalData->GetName());
    const int correctCellNumber = 10 * 10 * 10;
    TSM_ASSERT_EQUALS("The number of signal values generated is incorrect.", correctCellNumber, signalData->GetSize());
    product->Delete();
  }

  void testProgressUpdating()
  {
    MockProgressAction mockProgressAction;
    //Expectation checks that progress should be >= 0 and <= 100 and called at least once!
    EXPECT_CALL(mockProgressAction, eventRaised(AllOf(Le(100),Ge(0)))).Times(AtLeast(1));

    MDHistoWorkspace_sptr ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 3);
    vtkMDHistoHexFactory factory(ThresholdRange_scptr(new NoThresholdRange), "signal");

    factory.initialize(ws_sptr);
    vtkDataSet* product= factory.create(mockProgressAction);

    TSM_ASSERT("Progress Updates not used as expected.", Mock::VerifyAndClearExpectations(&mockProgressAction));
    product->Delete();
  }

  void testIsValidThrowsWhenNoWorkspace()
  {
    IMDWorkspace* nullWorkspace = NULL;
    Mantid::API::IMDWorkspace_sptr ws_sptr(nullWorkspace);
    
    vtkMDHistoHexFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 10000)), "signal");

    TSM_ASSERT_THROWS("No workspace, so should not be possible to complete initialization.", factory.initialize(ws_sptr), std::invalid_argument);
  }

  void testCreateWithoutInitializeThrows()
  {
    FakeProgressAction progressUpdate;
    vtkMDHistoHexFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 10000)), "signal");
    TS_ASSERT_THROWS(factory.create(progressUpdate), std::runtime_error);
  }

  void testInitializationDelegates()
  {
    //If the workspace provided is not a 4D imdworkspace, it should call the successor's initalization
    Mantid::API::IMDWorkspace_sptr ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2);

    MockvtkDataSetFactory* pMockFactorySuccessor = new MockvtkDataSetFactory;
    EXPECT_CALL(*pMockFactorySuccessor, initialize(_)).Times(1); //expect it then to call initialize on the successor.
    EXPECT_CALL(*pMockFactorySuccessor, getFactoryTypeName()).WillOnce(testing::Return("TypeA")); 

    //Constructional method ensures that factory is only suitable for providing mesh information.
    vtkMDHistoHexFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 10000)), "signal");

    //Successor is provided.
    factory.SetSuccessor(pMockFactorySuccessor);
    
    factory.initialize(ws_sptr);

    TSM_ASSERT("successor factory not used as expected.", Mock::VerifyAndClearExpectations(pMockFactorySuccessor));
  }

  void testInitializationDelegatesThrows()
  {
    //If the workspace provided is not a 4D imdworkspace, it should call the successor's initalization. If there is no successor an exception should be thrown.
    Mantid::API::IMDWorkspace_sptr ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2);

    //Constructional method ensures that factory is only suitable for providing mesh information.
    vtkMDHistoHexFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 10000)), "signal");

    TSM_ASSERT_THROWS("Should have thrown an execption given that no successor was available.", factory.initialize(ws_sptr), std::runtime_error);
  }

  void testCreateDelegates()
  {
    FakeProgressAction progressUpdate;
    //If the workspace provided is not a 4D imdworkspace, it should call the successor's initalization
    //2 dimensions on the workspace.
    Mantid::API::IMDWorkspace_sptr ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2);

    MockvtkDataSetFactory* pMockFactorySuccessor = new MockvtkDataSetFactory;
    EXPECT_CALL(*pMockFactorySuccessor, initialize(_)).Times(1); //expect it then to call initialize on the successor.
    EXPECT_CALL(*pMockFactorySuccessor, create(Ref(progressUpdate))).Times(1).WillOnce(Return(vtkStructuredGrid::New())); //expect it then to call create on the successor.
    EXPECT_CALL(*pMockFactorySuccessor, getFactoryTypeName()).WillOnce(testing::Return("TypeA")); 


    //Constructional method ensures that factory is only suitable for providing mesh information.
    vtkMDHistoHexFactory factory (ThresholdRange_scptr(new UserDefinedThresholdRange(0, 10000)), "signal");

    //Successor is provided.
    factory.SetSuccessor(pMockFactorySuccessor);
    
    factory.initialize(ws_sptr);
    factory.create(progressUpdate); // should be called on successor.

    TSM_ASSERT("successor factory not used as expected.", Mock::VerifyAndClearExpectations(pMockFactorySuccessor));
  }

  void testTypeName()
  {
    using namespace Mantid::VATES;
    vtkMDHistoHexFactory factory (ThresholdRange_scptr(new UserDefinedThresholdRange(0, 10000)), "signal");
    TS_ASSERT_EQUALS("vtkMDHistoHexFactory", factory.getFactoryTypeName());
  }

};

//=====================================================================================
// Performance tests
//=====================================================================================
class vtkMDHistoHexFactoryTestPerformance : public CxxTest::TestSuite
{
private:

  Mantid::API::IMDWorkspace_sptr m_ws_sptr;

public:

  void setUp()
  {
    //Create the workspace. 20 bins in each dimension.
    m_ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 3, 100);
    m_ws_sptr->setTransformFromOriginal(new NullCoordTransform);
  }

	void testGenerateHexahedronVtkDataSet()
	{
    FakeProgressAction progressUpdate;

    //Create the factory.
    vtkMDHistoHexFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 10000)), "signal");
    factory.initialize(m_ws_sptr);

    TS_ASSERT_THROWS_NOTHING(factory.create(progressUpdate));
	}
};

#endif
