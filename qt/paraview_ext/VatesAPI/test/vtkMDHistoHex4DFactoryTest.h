// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef VTK_MD_HEX_4D_FACTORY_TEST_H_
#define VTK_MD_HEX_4D_FACTORY_TEST_H_

#include "MantidAPI/IMDWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidKernel/make_unique.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidVatesAPI/TimeStepToTimeStep.h"
#include "MantidVatesAPI/vtkMDHistoHex4DFactory.h"
#include "MantidVatesAPI/vtkStructuredGrid_Silent.h"
#include "MockObjects.h"
#include <cxxtest/TestSuite.h>
#include <vtkSmartPointer.h>

using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::VATES;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace testing;

//=====================================================================================
// Functional Tests
//=====================================================================================
class vtkMDHistoHex4DFactoryTest : public CxxTest::TestSuite {

public:
  void testProgressUpdating() {
    MockProgressAction mockProgressAction;
    // Expectation checks that progress should be >= 0 and <= 100 and called at
    // least once!
    EXPECT_CALL(mockProgressAction, eventRaised(AllOf(Le(100), Ge(0))))
        .Times(AtLeast(1));

    MDHistoWorkspace_sptr ws_sptr =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 4);
    vtkMDHistoHex4DFactory<TimeStepToTimeStep> factory(
        Mantid::VATES::VolumeNormalization, 0);

    factory.initialize(ws_sptr);
    auto product = factory.create(mockProgressAction);

    TSM_ASSERT("Progress Updates not used as expected.",
               Mock::VerifyAndClearExpectations(&mockProgressAction));
  }

  void testSignalAspects() {
    FakeProgressAction progressUpdate;

    // Workspace with value 1.0 everywhere
    MDHistoWorkspace_sptr ws_sptr =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 4);
    ws_sptr->setTransformFromOriginal(new NullCoordTransform);

    // Constructional method ensures that factory is only suitable for providing
    // mesh information.
    vtkMDHistoHex4DFactory<TimeStepToTimeStep> factory =
        vtkMDHistoHex4DFactory<TimeStepToTimeStep>(
            Mantid::VATES::VolumeNormalization, 0);
    factory.initialize(ws_sptr);

    auto product = factory.create(progressUpdate);
    TSM_ASSERT_EQUALS("Two arrays (signal and cell ghost array) should be "
                      "present on the product dataset.",
                      2, product->GetCellData()->GetNumberOfArrays());
    auto signalData = vtkSmartPointer<vtkDataArray>::Take(
        product->GetCellData()->GetArray(0));
    TSM_ASSERT_EQUALS("The obtained cell data has the wrong name.",
                      std::string("signal"),
                      std::string(signalData->GetName()));
    const int correctCellNumber = 10 * 10 * 10;
    TSM_ASSERT_EQUALS("The number of signal values generated is incorrect.",
                      correctCellNumber, signalData->GetSize());
  }

  void testIsValidThrowsWhenNoWorkspace() {

    IMDWorkspace *nullWorkspace = nullptr;
    Mantid::API::IMDWorkspace_sptr ws_sptr(nullWorkspace);
    vtkMDHistoHex4DFactory<TimeStepToTimeStep> factory(
        Mantid::VATES::VolumeNormalization, 1);

    TSM_ASSERT_THROWS(
        "No workspace, so should not be possible to complete initialization.",
        factory.initialize(ws_sptr), const std::invalid_argument &);
  }

  void testCreateWithoutInitializeThrows() {
    FakeProgressAction progressAction;

    vtkMDHistoHex4DFactory<TimeStepToTimeStep> factory(
        Mantid::VATES::VolumeNormalization, 1);
    TS_ASSERT_THROWS(factory.create(progressAction),
                     const std::runtime_error &);
  }

  void testInitializationDelegates() {
    // If the workspace provided is not a 4D imdworkspace, it should call the
    // successor's initalization
    // 2D workspace
    MDHistoWorkspace_sptr ws_sptr =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2);

    auto pMockFactorySuccessor = new MockvtkDataSetFactory();
    auto uniqueSuccessor =
        std::unique_ptr<MockvtkDataSetFactory>(pMockFactorySuccessor);
    EXPECT_CALL(*pMockFactorySuccessor, initialize(_))
        .Times(1); // expect it then to call initialize on the successor.
    EXPECT_CALL(*pMockFactorySuccessor, getFactoryTypeName())
        .WillOnce(testing::Return("TypeA"));

    // Constructional method ensures that factory is only suitable for providing
    // mesh information.
    vtkMDHistoHex4DFactory<TimeStepToTimeStep> factory =
        vtkMDHistoHex4DFactory<TimeStepToTimeStep>(
            Mantid::VATES::VolumeNormalization, (double)0);

    // Successor is provided.
    factory.setSuccessor(std::move(uniqueSuccessor));

    factory.initialize(ws_sptr);

    // Need the raw pointer to test assertions here. Object is not yet deleted
    // as the factory is still in scope.
    TSM_ASSERT("successor factory not used as expected.",
               Mock::VerifyAndClearExpectations(pMockFactorySuccessor));
  }

  void testInitializationDelegatesThrows() {
    // If the workspace provided is not a 4D imdworkspace, it should call the
    // successor's initalization. If there is no successor an exception should
    // be thrown.
    // 2D workspace
    MDHistoWorkspace_sptr ws_sptr =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2);

    // Constructional method ensures that factory is only suitable for providing
    // mesh information.
    vtkMDHistoHex4DFactory<TimeStepToTimeStep> factory =
        vtkMDHistoHex4DFactory<TimeStepToTimeStep>(
            Mantid::VATES::VolumeNormalization, (double)0);

    TSM_ASSERT_THROWS("Should have thrown an execption given that no successor "
                      "was available.",
                      factory.initialize(ws_sptr), const std::runtime_error &);
  }

  void testCreateDelegates() {
    FakeProgressAction progressUpdate;

    // If the workspace provided is not a 4D imdworkspace, it should call the
    // successor's initalization
    // 2D workspace
    MDHistoWorkspace_sptr ws_sptr =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2);

    auto pMockFactorySuccessor = new MockvtkDataSetFactory();
    auto uniqueSuccessor =
        std::unique_ptr<MockvtkDataSetFactory>(pMockFactorySuccessor);
    EXPECT_CALL(*pMockFactorySuccessor, initialize(_))
        .Times(1); // expect it then to call initialize on the successor.
    EXPECT_CALL(*pMockFactorySuccessor, create(Ref(progressUpdate)))
        .Times(1)
        .WillOnce(
            Return(vtkSmartPointer<vtkStructuredGrid>::New())); // expect it
                                                                // then to call
                                                                // create on the
                                                                // successor.
    EXPECT_CALL(*pMockFactorySuccessor, getFactoryTypeName())
        .WillOnce(testing::Return("TypeA"));

    // Constructional method ensures that factory is only suitable for providing
    // mesh information.
    vtkMDHistoHex4DFactory<TimeStepToTimeStep> factory =
        vtkMDHistoHex4DFactory<TimeStepToTimeStep>(
            Mantid::VATES::VolumeNormalization, (double)0);

    // Successor is provided.
    factory.setSuccessor(std::move(uniqueSuccessor));

    factory.initialize(ws_sptr);
    factory.create(progressUpdate); // should be called on successor.

    // Need the raw pointer to test assertions here. Object is not yet deleted
    // as the factory is still in scope.
    TSM_ASSERT("successor factory not used as expected.",
               Mock::VerifyAndClearExpectations(pMockFactorySuccessor));
  }

  void testTypeName() {
    using namespace Mantid::VATES;

    vtkMDHistoHex4DFactory<TimeStepToTimeStep> factory =
        vtkMDHistoHex4DFactory<TimeStepToTimeStep>(
            Mantid::VATES::VolumeNormalization, (double)0);
    TS_ASSERT_EQUALS("vtkMDHistoHex4DFactory", factory.getFactoryTypeName());
  }
};

//=====================================================================================
// Performance Tests
//=====================================================================================
class vtkMDHistoHex4DFactoryTestPerformance : public CxxTest::TestSuite {
private:
  Mantid::API::IMDWorkspace_sptr m_ws_sptr;

public:
  void setUp() override {
    // Create a 4D workspace 50 ^ 4
    m_ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 4, 50);
    m_ws_sptr->setTransformFromOriginal(new NullCoordTransform);
  }

  void testGenerateVTKDataSet() {
    FakeProgressAction progressUpdate;

    vtkMDHistoHex4DFactory<TimeStepToTimeStep> factory(
        Mantid::VATES::VolumeNormalization, 0);
    factory.initialize(m_ws_sptr);
    TS_ASSERT_THROWS_NOTHING(factory.create(progressUpdate));
  }
};

#endif
