// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef VTK_MD_LINE_FACTORY_TEST
#define VTK_MD_LINE_FACTORY_TEST

#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/make_unique.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidVatesAPI/vtkMDLineFactory.h"
#include "MantidVatesAPI/vtkStructuredGrid_Silent.h"
#include "MockObjects.h"
#include "vtkCellType.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::VATES;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class vtkMDLineFactoryTest : public CxxTest::TestSuite {
public:
  void testGetFactoryTypeName() {
    vtkMDLineFactory factory(Mantid::VATES::VolumeNormalization);
    TS_ASSERT_EQUALS("vtkMDLineFactory", factory.getFactoryTypeName());
  }

  void testInitializeDelegatesToSuccessor() {
    auto mockSuccessor = new MockvtkDataSetFactory();
    auto uniqueSuccessor =
        std::unique_ptr<MockvtkDataSetFactory>(mockSuccessor);
    EXPECT_CALL(*mockSuccessor, initialize(_)).Times(1);
    EXPECT_CALL(*mockSuccessor, getFactoryTypeName()).Times(1);

    vtkMDLineFactory factory(Mantid::VATES::VolumeNormalization);
    factory.setSuccessor(std::move(uniqueSuccessor));

    ITableWorkspace_sptr ws =
        boost::make_shared<Mantid::DataObjects::TableWorkspace>();
    TS_ASSERT_THROWS_NOTHING(factory.initialize(ws));

    TSM_ASSERT("Successor has not been used properly.",
               Mock::VerifyAndClearExpectations(mockSuccessor));
  }

  void testCreateDelegatesToSuccessor() {
    FakeProgressAction progressUpdate;

    auto mockSuccessor = new MockvtkDataSetFactory();
    auto uniqueSuccessor =
        std::unique_ptr<MockvtkDataSetFactory>(mockSuccessor);
    EXPECT_CALL(*mockSuccessor, initialize(_)).Times(1);
    EXPECT_CALL(*mockSuccessor, create(Ref(progressUpdate)))
        .Times(1)
        .WillOnce(Return(vtkSmartPointer<vtkStructuredGrid>::New()));
    EXPECT_CALL(*mockSuccessor, getFactoryTypeName()).Times(1);

    vtkMDLineFactory factory(Mantid::VATES::VolumeNormalization);
    factory.setSuccessor(std::move(uniqueSuccessor));

    auto ws = boost::make_shared<Mantid::DataObjects::TableWorkspace>();
    TS_ASSERT_THROWS_NOTHING(factory.initialize(ws));
    TS_ASSERT_THROWS_NOTHING(factory.create(progressUpdate));

    TSM_ASSERT("Successor has not been used properly.",
               Mock::VerifyAndClearExpectations(mockSuccessor));
  }

  void testOnInitaliseCannotDelegateToSuccessor() {
    vtkMDLineFactory factory(Mantid::VATES::VolumeNormalization);
    ITableWorkspace_sptr ws =
        boost::make_shared<Mantid::DataObjects::TableWorkspace>();
    TS_ASSERT_THROWS(factory.initialize(ws), const std::runtime_error &);
  }

  void testCreateWithoutInitializeThrows() {
    FakeProgressAction progressUpdate;

    vtkMDLineFactory factory(Mantid::VATES::VolumeNormalization);
    // initialize not called!
    TS_ASSERT_THROWS(factory.create(progressUpdate), const std::runtime_error &);
  }

  void testCreation() {
    MockProgressAction mockProgressAction;
    // Expectation checks that progress should be >= 0 and <= 100 and called at
    // least once!
    EXPECT_CALL(mockProgressAction, eventRaised(AllOf(Le(100), Ge(0))))
        .Times(AtLeast(1));

    boost::shared_ptr<Mantid::DataObjects::MDEventWorkspace<
        Mantid::DataObjects::MDEvent<1>, 1>>
        ws = MDEventsTestHelper::makeMDEWFull<1>(10, 10, 10, 10);

    // Rebin it to make it possible to compare cells to bins.
    using namespace Mantid::API;
    IAlgorithm_sptr slice =
        AlgorithmManager::Instance().createUnmanaged("SliceMD");
    slice->initialize();
    slice->setProperty("InputWorkspace", ws);
    slice->setPropertyValue("AlignedDim0", "Axis0, -10, 10, 100");
    slice->setPropertyValue("OutputWorkspace", "binned");
    slice->execute();

    Workspace_sptr binned =
        Mantid::API::AnalysisDataService::Instance().retrieve("binned");

    vtkMDLineFactory factory(Mantid::VATES::VolumeNormalization);
    factory.initialize(binned);

    auto product = factory.create(mockProgressAction);

    TS_ASSERT(dynamic_cast<vtkUnstructuredGrid *>(product.GetPointer()) !=
              NULL);
    TS_ASSERT_EQUALS(100, product->GetNumberOfCells());
    TS_ASSERT_EQUALS(200, product->GetNumberOfPoints());
    TS_ASSERT_EQUALS(VTK_LINE, product->GetCellType(0));

    AnalysisDataService::Instance().remove("binned");
    TSM_ASSERT("Progress Updates not used as expected.",
               Mock::VerifyAndClearExpectations(&mockProgressAction));
  }
};

//=====================================================================================
// Peformance tests
//=====================================================================================
class vtkMDLineFactoryTestPerformance : public CxxTest::TestSuite {

public:
  void setUp() override {
    boost::shared_ptr<Mantid::DataObjects::MDEventWorkspace<
        Mantid::DataObjects::MDEvent<1>, 1>>
        input = MDEventsTestHelper::makeMDEWFull<1>(2, 10, 10, 4000);
    // Rebin it to make it possible to compare cells to bins.
    using namespace Mantid::API;
    IAlgorithm_sptr slice =
        AlgorithmManager::Instance().createUnmanaged("SliceMD");
    slice->initialize();
    slice->setProperty("InputWorkspace", input);
    slice->setPropertyValue("AlignedDim0", "Axis0, -10, 10, 200000");
    slice->setPropertyValue("OutputWorkspace", "binned");
    slice->execute();
  }

  void tearDown() override { AnalysisDataService::Instance().remove("binned"); }

  void testCreationOnLargeWorkspace() {
    FakeProgressAction progressAction;

    Workspace_sptr binned =
        Mantid::API::AnalysisDataService::Instance().retrieve("binned");

    vtkMDLineFactory factory(Mantid::VATES::VolumeNormalization);
    factory.initialize(binned);

    auto product = factory.create(progressAction);

    TS_ASSERT(dynamic_cast<vtkUnstructuredGrid *>(product.GetPointer()) !=
              nullptr);
    TS_ASSERT_EQUALS(200000, product->GetNumberOfCells());
    TS_ASSERT_EQUALS(400000, product->GetNumberOfPoints());
  }
};

#endif
