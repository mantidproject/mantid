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
#include <vtkSmartPointer.h>
#include "MantidKernel/make_unique.h"

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

    vtkMDHistoHex4DFactory<TimeStepToTimeStep> inside(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 2)), Mantid::VATES::VolumeNormalization, 0);
    inside.initialize(ws_sptr);
    auto insideData = inside.create(progressAction);
    auto insideProduct = vtkStructuredGrid::SafeDownCast(insideData.Get());

    vtkMDHistoHex4DFactory<TimeStepToTimeStep> below(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 0.5)), Mantid::VATES::VolumeNormalization, 0);
    below.initialize(ws_sptr);
    auto belowData = below.create(progressAction);
    auto belowProduct = vtkStructuredGrid::SafeDownCast(belowData.Get());

    vtkMDHistoHex4DFactory<TimeStepToTimeStep> above(ThresholdRange_scptr(new UserDefinedThresholdRange(2, 3)), Mantid::VATES::VolumeNormalization, 0);
    above.initialize(ws_sptr);
    auto aboveData = above.create(progressAction);
    auto aboveProduct = vtkStructuredGrid::SafeDownCast(aboveData.Get());

    TS_ASSERT_EQUALS((10*10*10), insideProduct->GetNumberOfCells());
    for (auto i = 0; i < insideProduct->GetNumberOfCells(); ++i) {
      TS_ASSERT(insideProduct->IsCellVisible(i) != 0);
    }

    // This has changed. Cells are still present but not visible.
    TS_ASSERT_EQUALS((10 * 10 * 10), belowProduct->GetNumberOfCells());
    for (auto i = 0; i < belowProduct->GetNumberOfCells(); ++i) {
      TS_ASSERT(belowProduct->IsCellVisible(i) == 0);
    }

    TS_ASSERT_EQUALS((10 * 10 * 10), aboveProduct->GetNumberOfCells());
    for (auto i = 0; i < aboveProduct->GetNumberOfCells(); ++i) {
      TS_ASSERT(aboveProduct->IsCellVisible(i) == 0);
    }
  }

  void testProgressUpdating()
  {
    MockProgressAction mockProgressAction;
    //Expectation checks that progress should be >= 0 and <= 100 and called at least once!
    EXPECT_CALL(mockProgressAction, eventRaised(AllOf(Le(100),Ge(0)))).Times(AtLeast(1));

    MDHistoWorkspace_sptr ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 4);
    vtkMDHistoHex4DFactory<TimeStepToTimeStep> factory(ThresholdRange_scptr(new NoThresholdRange), Mantid::VATES::VolumeNormalization, 0);

    factory.initialize(ws_sptr);
    auto product = factory.create(mockProgressAction);

    TSM_ASSERT("Progress Updates not used as expected.", Mock::VerifyAndClearExpectations(&mockProgressAction));
  }

  void testSignalAspects() {
    FakeProgressAction progressUpdate;

    // Workspace with value 1.0 everywhere
    MDHistoWorkspace_sptr ws_sptr =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 4);
    ws_sptr->setTransformFromOriginal(new NullCoordTransform);
    auto pRange =
        Mantid::Kernel::make_unique<UserDefinedThresholdRange>(0, 100);

    // Constructional method ensures that factory is only suitable for providing
    // mesh information.
    vtkMDHistoHex4DFactory<TimeStepToTimeStep> factory =
        vtkMDHistoHex4DFactory<TimeStepToTimeStep>(
            ThresholdRange_scptr(pRange.release()),
            Mantid::VATES::VolumeNormalization, 0);
    factory.initialize(ws_sptr);

    auto product = factory.create(progressUpdate);
    TSM_ASSERT_EQUALS(
        "A single array should be present on the product dataset.", 1,
        product->GetCellData()->GetNumberOfArrays());
    auto signalData = vtkSmartPointer<vtkDataArray>::Take(
        product->GetCellData()->GetArray(0));
    TSM_ASSERT_EQUALS("The obtained cell data has the wrong name.",
                      std::string("signal"),
                      std::string(signalData->GetName()));
    const int correctCellNumber = 10 * 10 * 10;
    TSM_ASSERT_EQUALS("The number of signal values generated is incorrect.",
                      correctCellNumber, signalData->GetSize());
  }

  void testIsValidThrowsWhenNoWorkspace()
  {

    IMDWorkspace* nullWorkspace = NULL;
    Mantid::API::IMDWorkspace_sptr ws_sptr(nullWorkspace);
    UserDefinedThresholdRange* pRange = new UserDefinedThresholdRange(0, 100);
    vtkMDHistoHex4DFactory<TimeStepToTimeStep> factory(ThresholdRange_scptr(pRange), Mantid::VATES::VolumeNormalization, 1);

    TSM_ASSERT_THROWS("No workspace, so should not be possible to complete initialization.", factory.initialize(ws_sptr), std::invalid_argument);
  }

  void testCreateWithoutInitializeThrows()
  {
    FakeProgressAction progressAction;

    auto pRange =
        Mantid::Kernel::make_unique<UserDefinedThresholdRange>(0, 100);
    vtkMDHistoHex4DFactory<TimeStepToTimeStep> factory(
        ThresholdRange_scptr(pRange.release()),
        Mantid::VATES::VolumeNormalization, 1);
    TS_ASSERT_THROWS(factory.create(progressAction), std::runtime_error);
  }

  void testInitializationDelegates()
  {
    //If the workspace provided is not a 4D imdworkspace, it should call the successor's initalization
    // 2D workspace
    MDHistoWorkspace_sptr ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2);

    auto pMockFactorySuccessor = new MockvtkDataSetFactory();
    auto uniqueSuccessor =
        std::unique_ptr<MockvtkDataSetFactory>(pMockFactorySuccessor);
    EXPECT_CALL(*pMockFactorySuccessor, initialize(_)).Times(1); //expect it then to call initialize on the successor.
    EXPECT_CALL(*pMockFactorySuccessor, getFactoryTypeName()).WillOnce(testing::Return("TypeA"));

    auto pRange =
        Mantid::Kernel::make_unique<UserDefinedThresholdRange>(0, 100);

    //Constructional method ensures that factory is only suitable for providing mesh information.
    vtkMDHistoHex4DFactory<TimeStepToTimeStep> factory =
        vtkMDHistoHex4DFactory<TimeStepToTimeStep>(
            ThresholdRange_scptr(pRange.release()),
            Mantid::VATES::VolumeNormalization, (double)0);

    //Successor is provided.
    factory.setSuccessor(std::move(uniqueSuccessor));

    factory.initialize(ws_sptr);

    // Need the raw pointer to test assertions here. Object is not yet deleted
    // as the factory is still in scope.
    TSM_ASSERT("successor factory not used as expected.",
               Mock::VerifyAndClearExpectations(pMockFactorySuccessor));
  }

  void testInitializationDelegatesThrows()
  {
    //If the workspace provided is not a 4D imdworkspace, it should call the successor's initalization. If there is no successor an exception should be thrown.
    // 2D workspace
    MDHistoWorkspace_sptr ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2);

    auto pRange =
        Mantid::Kernel::make_unique<UserDefinedThresholdRange>(0, 100);

    //Constructional method ensures that factory is only suitable for providing mesh information.
    vtkMDHistoHex4DFactory<TimeStepToTimeStep> factory =
        vtkMDHistoHex4DFactory<TimeStepToTimeStep>(
            ThresholdRange_scptr(pRange.release()),
            Mantid::VATES::VolumeNormalization, (double)0);

    TSM_ASSERT_THROWS("Should have thrown an execption given that no successor was available.", factory.initialize(ws_sptr), std::runtime_error);
  }

  void testCreateDelegates()
  {
    FakeProgressAction progressUpdate;

    //If the workspace provided is not a 4D imdworkspace, it should call the successor's initalization
    // 2D workspace
    MDHistoWorkspace_sptr ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2);

    auto pMockFactorySuccessor = new MockvtkDataSetFactory();
    auto uniqueSuccessor =
        std::unique_ptr<MockvtkDataSetFactory>(pMockFactorySuccessor);
    EXPECT_CALL(*pMockFactorySuccessor, initialize(_)).Times(1); //expect it then to call initialize on the successor.
    EXPECT_CALL(*pMockFactorySuccessor, create(Ref(progressUpdate)))
        .Times(1)
        .WillOnce(
            Return(vtkSmartPointer<vtkStructuredGrid>::New())); // expect it
                                                                // then to call
                                                                // create on the
                                                                // successor.
    EXPECT_CALL(*pMockFactorySuccessor, getFactoryTypeName()).WillOnce(testing::Return("TypeA"));

    auto pRange =
        Mantid::Kernel::make_unique<UserDefinedThresholdRange>(0, 100);

    //Constructional method ensures that factory is only suitable for providing mesh information.
    vtkMDHistoHex4DFactory<TimeStepToTimeStep> factory =
        vtkMDHistoHex4DFactory<TimeStepToTimeStep>(
            ThresholdRange_scptr(pRange.release()),
            Mantid::VATES::VolumeNormalization, (double)0);

    //Successor is provided.
    factory.setSuccessor(std::move(uniqueSuccessor));

    factory.initialize(ws_sptr);
    factory.create(progressUpdate); // should be called on successor.

    // Need the raw pointer to test assertions here. Object is not yet deleted
    // as the factory is still in scope.
    TSM_ASSERT("successor factory not used as expected.",
               Mock::VerifyAndClearExpectations(pMockFactorySuccessor));
  }

  void testTypeName()
  {
    using namespace Mantid::VATES;

    auto pRange =
        Mantid::Kernel::make_unique<UserDefinedThresholdRange>(0, 100);

    vtkMDHistoHex4DFactory<TimeStepToTimeStep> factory =
        vtkMDHistoHex4DFactory<TimeStepToTimeStep>(
            ThresholdRange_scptr(pRange.release()),
            Mantid::VATES::VolumeNormalization, (double)0);
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

    auto pRange =
        Mantid::Kernel::make_unique<UserDefinedThresholdRange>(0, 100000);
    vtkMDHistoHex4DFactory<TimeStepToTimeStep> factory(
        ThresholdRange_scptr(pRange.release()),
        Mantid::VATES::VolumeNormalization, 0);
    factory.initialize(m_ws_sptr);
    TS_ASSERT_THROWS_NOTHING(factory.create(progressUpdate));
  }

};


#endif
