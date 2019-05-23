// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef VTK_MD_HISTO_LINE_FACTORY_TEST_H_
#define VTK_MD_HISTO_LINE_FACTORY_TEST_H_

#include "MantidAPI/IMDIterator.h"
#include "MantidKernel/make_unique.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidVatesAPI/vtkMDHistoLineFactory.h"
#include "MantidVatesAPI/vtkStructuredGrid_Silent.h"
#include "MockObjects.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vtkSmartPointer.h>

using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::VATES;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class vtkMDHistoLineFactoryTest : public CxxTest::TestSuite {

public:
  void testIsValidThrowsWhenNoWorkspace() {
    IMDWorkspace *nullWorkspace = nullptr;
    Mantid::API::IMDWorkspace_sptr ws_sptr(nullWorkspace);

    vtkMDHistoLineFactory factory(Mantid::VATES::VolumeNormalization);

    TSM_ASSERT_THROWS(
        "No workspace, so should not be possible to complete initialization.",
        factory.initialize(ws_sptr), const std::invalid_argument &);
  }

  void testCreateWithoutInitializeThrows() {
    FakeProgressAction progressUpdate;
    vtkMDHistoLineFactory factory(Mantid::VATES::VolumeNormalization);
    TS_ASSERT_THROWS(factory.create(progressUpdate),
                     const std::runtime_error &);
  }

  void testProgressUpdates() {
    MockProgressAction mockProgressAction;
    // Expectation checks that progress should be >= 0 and <= 100 and called at
    // least once!
    EXPECT_CALL(mockProgressAction, eventRaised(AllOf(Le(100), Ge(0))))
        .Times(AtLeast(1));

    MDHistoWorkspace_sptr ws_sptr =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 1);
    vtkMDHistoLineFactory factory(Mantid::VATES::VolumeNormalization);

    factory.initialize(ws_sptr);
    auto product = factory.create(mockProgressAction);

    TSM_ASSERT("Progress Updates not used as expected.",
               Mock::VerifyAndClearExpectations(&mockProgressAction));
  }

  void testInitializationDelegates() {
    // If the workspace provided is not a 1D imdworkspace, it should call the
    // successor's initalization
    // 3 dimensions on the workspace
    Mantid::API::IMDWorkspace_sptr ws_sptr =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 3);

    auto pMockFactorySuccessor = new MockvtkDataSetFactory();
    auto uniqueSuccessor =
        std::unique_ptr<MockvtkDataSetFactory>(pMockFactorySuccessor);
    EXPECT_CALL(*pMockFactorySuccessor, initialize(_))
        .Times(1); // expect it then to call initialize on the successor.
    EXPECT_CALL(*pMockFactorySuccessor, getFactoryTypeName())
        .WillOnce(testing::Return("TypeA"));

    // Constructional method ensures that factory is only suitable for providing
    // mesh information.
    vtkMDHistoLineFactory factory(Mantid::VATES::VolumeNormalization);

    // Successor is provided.
    factory.setSuccessor(std::move(uniqueSuccessor));
    factory.initialize(ws_sptr);

    // Need the raw pointer to test assertions here. Object is not yet deleted
    // as the factory is still in scope.
    TSM_ASSERT("successor factory not used as expected.",
               Mock::VerifyAndClearExpectations(pMockFactorySuccessor));
  }

  void testInitializationDelegatesThrows() {
    // If the workspace provided is not a 2D imdworkspace, it should call the
    // successor's initalization. If there is no successor an exception should
    // be thrown.
    // 3 dimensions on the workspace
    Mantid::API::IMDWorkspace_sptr ws_sptr =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 3);

    // Constructional method ensures that factory is only suitable for providing
    // mesh information.
    vtkMDHistoLineFactory factory(Mantid::VATES::VolumeNormalization);

    TSM_ASSERT_THROWS("Should have thrown an execption given that no successor "
                      "was available.",
                      factory.initialize(ws_sptr), const std::runtime_error &);
  }

  void testCreateDelegates() {
    FakeProgressAction progressUpdate;
    // If the workspace provided is not a 2D imdworkspace, it should call the
    // successor's initalization
    // 3 dimensions on the workspace
    Mantid::API::IMDWorkspace_sptr ws_sptr =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 3);

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
    vtkMDHistoLineFactory factory(Mantid::VATES::VolumeNormalization);

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
    vtkMDHistoLineFactory factory(Mantid::VATES::VolumeNormalization);
    TS_ASSERT_EQUALS("vtkMDHistoLineFactory", factory.getFactoryTypeName());
  }
};

//=====================================================================================
// Performance tests
//=====================================================================================
class vtkMDHistoLineFactoryTestPerformance : public CxxTest::TestSuite {

private:
  Mantid::API::IMDWorkspace_sptr m_ws_sptr;

public:
  void setUp() override {
    // 1D Workspace with 2000 points
    m_ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 1, 200000);
  }

  void testGenerateVTKDataSet() {
    FakeProgressAction progressUpdate;
    vtkMDHistoLineFactory factory(Mantid::VATES::VolumeNormalization);
    factory.initialize(m_ws_sptr);
    TS_ASSERT_THROWS_NOTHING(factory.create(progressUpdate));
  }
};

#endif
