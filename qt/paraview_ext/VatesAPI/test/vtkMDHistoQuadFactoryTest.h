// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef VTK_MD_QUAD_FACTORY_TEST_H_
#define VTK_MD_QUAD_FACTORY_TEST_H_

#include "MantidAPI/IMDIterator.h"
#include "MantidKernel/make_unique.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidVatesAPI/vtkMDHistoQuadFactory.h"
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
using Mantid::DataObjects::MDEventsTestHelper::makeFakeMDHistoWorkspace;

//=====================================================================================
// Functional tests
//=====================================================================================
class vtkMDHistoQuadFactoryTest : public CxxTest::TestSuite {

public:
  void testIsValidThrowsWhenNoWorkspace() {
    using namespace Mantid::VATES;
    using namespace Mantid::API;

    IMDWorkspace *nullWorkspace = nullptr;
    Mantid::API::IMDWorkspace_sptr ws_sptr(nullWorkspace);

    vtkMDHistoQuadFactory factory(Mantid::VATES::VolumeNormalization);

    TSM_ASSERT_THROWS(
        "No workspace, so should not be possible to complete initialization.",
        factory.initialize(ws_sptr), const std::invalid_argument &);
  }

  void testCreateWithoutInitializeThrows() {
    FakeProgressAction progressUpdate;

    vtkMDHistoQuadFactory factory(Mantid::VATES::VolumeNormalization);
    TS_ASSERT_THROWS(factory.create(progressUpdate), const std::runtime_error &);
  }

  void testInsideThresholds() {
    FakeProgressAction progressUpdate;

    // WS with 2 dimensions
    Mantid::API::IMDWorkspace_sptr ws_sptr =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2);

    vtkMDHistoQuadFactory inside(Mantid::VATES::VolumeNormalization);
    inside.initialize(ws_sptr);
    auto product = inside.create(progressUpdate);
    auto data = vtkDataSet::SafeDownCast(product.Get());
    vtkSmartPointer<vtkDataSet> insideProduct(data);

    TS_ASSERT_EQUALS((10 * 10), insideProduct->GetNumberOfCells());
    TS_ASSERT_EQUALS((11 * 11), insideProduct->GetNumberOfPoints());
  }

  void testInitializationDelegates() {
    // If the workspace provided is not a 4D imdworkspace, it should call the
    // successor's initalization
    // WS with 1 dimension
    Mantid::API::IMDWorkspace_sptr ws_sptr =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 1);

    auto pMockFactorySuccessor = new MockvtkDataSetFactory();
    auto uniqueSuccessor =
        std::unique_ptr<MockvtkDataSetFactory>(pMockFactorySuccessor);
    EXPECT_CALL(*pMockFactorySuccessor, getFactoryTypeName())
        .WillOnce(testing::Return("TypeA"));
    EXPECT_CALL(*pMockFactorySuccessor, initialize(_))
        .Times(1); // expect it then to call initialize on the successor.

    // Constructional method ensures that factory is only suitable for providing
    // mesh information.
    vtkMDHistoQuadFactory factory(Mantid::VATES::VolumeNormalization);

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
    // WS with 1 dimension
    Mantid::API::IMDWorkspace_sptr ws_sptr =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 1);

    // Constructional method ensures that factory is only suitable for providing
    // mesh information.
    vtkMDHistoQuadFactory factory(Mantid::VATES::VolumeNormalization);

    TSM_ASSERT_THROWS("Should have thrown an execption given that no successor "
                      "was available.",
                      factory.initialize(ws_sptr), const std::runtime_error &);
  }

  void testCreateDelegates() {
    FakeProgressAction progressUpdate;
    // If the workspace provided is not a 2D imdworkspace, it should call the
    // successor's initalization
    // WS with 1 dimension
    Mantid::API::IMDWorkspace_sptr ws_sptr =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 1);

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
    vtkMDHistoQuadFactory factory(Mantid::VATES::VolumeNormalization);

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
    vtkMDHistoQuadFactory factory(Mantid::VATES::VolumeNormalization);
    TS_ASSERT_EQUALS("vtkMDHistoQuadFactory", factory.getFactoryTypeName());
  }

  void testProgressUpdates() {
    MockProgressAction mockProgressAction;
    // Expectation checks that progress should be >= 0 and <= 100 and called at
    // least once!
    EXPECT_CALL(mockProgressAction, eventRaised(AllOf(Le(100), Ge(0))))
        .Times(AtLeast(1));

    MDHistoWorkspace_sptr ws_sptr =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2);
    vtkMDHistoQuadFactory factory(Mantid::VATES::VolumeNormalization);

    factory.initialize(ws_sptr);
    auto product = factory.create(mockProgressAction);

    TSM_ASSERT("Progress Updates not used as expected.",
               Mock::VerifyAndClearExpectations(&mockProgressAction));
  }
};

//=====================================================================================
// Performance tests
//=====================================================================================
class vtkMDHistoQuadFactoryTestPerformance : public CxxTest::TestSuite {
private:
  Mantid::API::IMDWorkspace_sptr m_ws_sptr;

public:
  void setUp() override {
    // WS with 2 dimension, 100x100
    m_ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2, 1000);
    m_ws_sptr->setTransformFromOriginal(new NullCoordTransform);
  }

  void testGenerateVTKDataSet() {
    FakeProgressAction progressUpdate;
    vtkMDHistoQuadFactory factory(Mantid::VATES::VolumeNormalization);
    factory.initialize(m_ws_sptr);
    TS_ASSERT_THROWS_NOTHING(factory.create(progressUpdate));
  }
};

#endif
