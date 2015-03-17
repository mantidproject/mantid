#ifndef VTK_MD_HEX_4D_FACTORY_TEST_H_
#define VTK_MD_HEX_4D_FACTORY_TEST_H_

#include "MantidAPI/IMDWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidVatesAPI/TimeStepToTimeStep.h"
#include "MantidVatesAPI/UserDefinedThresholdRange.h"
#include "MantidVatesAPI/vtkMDHistoHex4DFactory.h"
#include "MantidVatesAPI/NoThresholdRange.h"
#include "MockObjects.h"
#include <cxxtest/TestSuite.h>
#include "MantidVatesAPI/vtkStructuredGrid_Silent.h"

using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::VATES;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace testing;

//=====================================================================================
// Functional Tests
//=====================================================================================
class vtkMDHistoHex4DFactoryTest: public CxxTest::TestSuite
{

public:

  void testThresholds()
  {
    FakeProgressAction progressAction;

    // Workspace with value 1.0 everywhere
    MDHistoWorkspace_sptr ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 4);
    ws_sptr->setTransformFromOriginal(new NullCoordTransform);

    //Set up so that only cells with signal values == 1 should not be filtered out by thresholding.

    vtkMDHistoHex4DFactory<TimeStepToTimeStep> inside(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 2)), "signal", 0);
    inside.initialize(ws_sptr);
    vtkUnstructuredGrid* insideProduct = dynamic_cast<vtkUnstructuredGrid*>(inside.create(progressAction));

    vtkMDHistoHex4DFactory<TimeStepToTimeStep> below(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 0.5)),"signal", 0);
    below.initialize(ws_sptr);
    vtkUnstructuredGrid* belowProduct = dynamic_cast<vtkUnstructuredGrid*>(below.create(progressAction));

    vtkMDHistoHex4DFactory<TimeStepToTimeStep> above(ThresholdRange_scptr(new UserDefinedThresholdRange(2, 3)), "signal", 0);
    above.initialize(ws_sptr);
    vtkUnstructuredGrid* aboveProduct = dynamic_cast<vtkUnstructuredGrid*>(above.create(progressAction));

    TS_ASSERT_EQUALS((10*10*10), insideProduct->GetNumberOfCells());
    TS_ASSERT_EQUALS(0, belowProduct->GetNumberOfCells());
    TS_ASSERT_EQUALS(0, aboveProduct->GetNumberOfCells());
  }

  void testProgressUpdating()
  {
    MockProgressAction mockProgressAction;
    //Expectation checks that progress should be >= 0 and <= 100 and called at least once!
    EXPECT_CALL(mockProgressAction, eventRaised(AllOf(Le(100),Ge(0)))).Times(AtLeast(1));

    MDHistoWorkspace_sptr ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 4);
    vtkMDHistoHex4DFactory<TimeStepToTimeStep> factory(ThresholdRange_scptr(new NoThresholdRange), "signal", 0);

    factory.initialize(ws_sptr);
    vtkDataSet* product= factory.create(mockProgressAction);

    TSM_ASSERT("Progress Updates not used as expected.", Mock::VerifyAndClearExpectations(&mockProgressAction));
    product->Delete();
  }

  void testSignalAspects()
  {
    FakeProgressAction progressUpdate;

    // Workspace with value 1.0 everywhere
    MDHistoWorkspace_sptr ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 4);
    ws_sptr->setTransformFromOriginal(new NullCoordTransform);
    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(0, 100);

    //Constructional method ensures that factory is only suitable for providing mesh information.
    vtkMDHistoHex4DFactory<TimeStepToTimeStep> factory =
      vtkMDHistoHex4DFactory<TimeStepToTimeStep> (ThresholdRange_scptr(pRange), "signal", 0);
    factory.initialize(ws_sptr);

    vtkDataSet* product = factory.create(progressUpdate);
    TSM_ASSERT_EQUALS("A single array should be present on the product dataset.", 1, product->GetCellData()->GetNumberOfArrays());
    vtkDataArray* signalData = product->GetCellData()->GetArray(0);
    TSM_ASSERT_EQUALS("The obtained cell data has the wrong name.", std::string("signal"), signalData->GetName());
    const int correctCellNumber = 10*10*10;
    TSM_ASSERT_EQUALS("The number of signal values generated is incorrect.", correctCellNumber, signalData->GetSize());
    
    product->Delete();
  }

  void testIsValidThrowsWhenNoWorkspace()
  {

    IMDWorkspace* nullWorkspace = NULL;
    Mantid::API::IMDWorkspace_sptr ws_sptr(nullWorkspace);
    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(0, 100);
    vtkMDHistoHex4DFactory<TimeStepToTimeStep> factory(ThresholdRange_scptr(pRange), "signal", 1);

    TSM_ASSERT_THROWS("No workspace, so should not be possible to complete initialization.", factory.initialize(ws_sptr), std::invalid_argument);
  }

  void testCreateWithoutInitializeThrows()
  {
    FakeProgressAction progressAction;

    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(0, 100);
    vtkMDHistoHex4DFactory<TimeStepToTimeStep> factory(ThresholdRange_scptr(pRange), "signal", 1);
    TS_ASSERT_THROWS(factory.create(progressAction), std::runtime_error);
  }

  void testInitializationDelegates()
  {
    //If the workspace provided is not a 4D imdworkspace, it should call the successor's initalization
    // 2D workspace
    MDHistoWorkspace_sptr ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2);

    MockvtkDataSetFactory* pMockFactorySuccessor = new MockvtkDataSetFactory;
    EXPECT_CALL(*pMockFactorySuccessor, initialize(_)).Times(1); //expect it then to call initialize on the successor.
    EXPECT_CALL(*pMockFactorySuccessor, getFactoryTypeName()).WillOnce(testing::Return("TypeA")); 

    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(0, 100);

    //Constructional method ensures that factory is only suitable for providing mesh information.
    vtkMDHistoHex4DFactory<TimeStepToTimeStep> factory =
      vtkMDHistoHex4DFactory<TimeStepToTimeStep> (ThresholdRange_scptr(pRange), "signal", (double)0);

    //Successor is provided.
    factory.SetSuccessor(pMockFactorySuccessor);

    factory.initialize(ws_sptr);

    TSM_ASSERT("successor factory not used as expected.", Mock::VerifyAndClearExpectations(pMockFactorySuccessor));
  }

  void testInitializationDelegatesThrows()
  {
    //If the workspace provided is not a 4D imdworkspace, it should call the successor's initalization. If there is no successor an exception should be thrown.
    // 2D workspace
    MDHistoWorkspace_sptr ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2);

    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(0, 100);

    //Constructional method ensures that factory is only suitable for providing mesh information.
    vtkMDHistoHex4DFactory<TimeStepToTimeStep> factory =
      vtkMDHistoHex4DFactory<TimeStepToTimeStep> (ThresholdRange_scptr(pRange), "signal", (double)0);

    TSM_ASSERT_THROWS("Should have thrown an execption given that no successor was available.", factory.initialize(ws_sptr), std::runtime_error);
  }

  void testCreateDelegates()
  {
    FakeProgressAction progressUpdate;

    //If the workspace provided is not a 4D imdworkspace, it should call the successor's initalization
    // 2D workspace
    MDHistoWorkspace_sptr ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2);

    MockvtkDataSetFactory* pMockFactorySuccessor = new MockvtkDataSetFactory;
    EXPECT_CALL(*pMockFactorySuccessor, initialize(_)).Times(1); //expect it then to call initialize on the successor.
    EXPECT_CALL(*pMockFactorySuccessor, create(Ref(progressUpdate))).Times(1).WillOnce(Return(vtkStructuredGrid::New())); //expect it then to call create on the successor.
    EXPECT_CALL(*pMockFactorySuccessor, getFactoryTypeName()).WillOnce(testing::Return("TypeA")); 

    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(0, 100);

    //Constructional method ensures that factory is only suitable for providing mesh information.
    vtkMDHistoHex4DFactory<TimeStepToTimeStep> factory =
      vtkMDHistoHex4DFactory<TimeStepToTimeStep> (ThresholdRange_scptr(pRange), "signal", (double)0);

    //Successor is provided.
    factory.SetSuccessor(pMockFactorySuccessor);

    factory.initialize(ws_sptr);
    factory.create(progressUpdate); // should be called on successor.

    TSM_ASSERT("successor factory not used as expected.", Mock::VerifyAndClearExpectations(pMockFactorySuccessor));
  }

  void testTypeName()
  {
    using namespace Mantid::VATES;

    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(0, 100);

    vtkMDHistoHex4DFactory<TimeStepToTimeStep> factory =
      vtkMDHistoHex4DFactory<TimeStepToTimeStep> (ThresholdRange_scptr(pRange), "signal", (double)0);
    TS_ASSERT_EQUALS("vtkMDHistoHex4DFactory", factory.getFactoryTypeName());
  }

};

//=====================================================================================
// Performance Tests
//=====================================================================================
class vtkMDHistoHex4DFactoryTestPerformance : public CxxTest::TestSuite
{
private:

  Mantid::API::IMDWorkspace_sptr m_ws_sptr;

public:

  void setUp()
  {
    //Create a 4D workspace 50 ^ 4
    m_ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 4, 50);
    m_ws_sptr->setTransformFromOriginal(new NullCoordTransform);
  }

  void testGenerateVTKDataSet()
  {
    FakeProgressAction progressUpdate;

    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(0, 100000);
    vtkMDHistoHex4DFactory<TimeStepToTimeStep> factory(ThresholdRange_scptr(pRange), "signal", 0);
    factory.initialize(m_ws_sptr);
    TS_ASSERT_THROWS_NOTHING(factory.create(progressUpdate));
  }

};


#endif
