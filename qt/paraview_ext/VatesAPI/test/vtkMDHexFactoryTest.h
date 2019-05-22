// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef VTK_MD_HEX_FACTORY_TEST
#define VTK_MD_HEX_FACTORY_TEST

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/make_unique.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidVatesAPI/vtkMDHexFactory.h"
#include "MantidVatesAPI/vtkStructuredGrid_Silent.h"
#include "MockObjects.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkSmartPointer.h>

using namespace Mantid;
using namespace Mantid::VATES;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class vtkMDHexFactoryTest : public CxxTest::TestSuite {
private:
  void doDimensionalityTesting(bool doCheckDimensionality) {
    Mantid::DataObjects::MDEventWorkspace3Lean::sptr input_ws =
        MDEventsTestHelper::makeMDEW<3>(10, 0.0, 10.0, 1);

    using namespace Mantid::API;
    IAlgorithm_sptr slice =
        AlgorithmManager::Instance().createUnmanaged("SliceMD");
    slice->initialize();
    slice->setProperty("InputWorkspace", input_ws);
    slice->setPropertyValue("AlignedDim0", "Axis0, -10, 10, 1");
    slice->setPropertyValue("AlignedDim0", "Axis1, -10, 10, 1");
    slice->setPropertyValue("AlignedDim0", "Axis2, -10, 10, 1");
    slice->setPropertyValue("OutputWorkspace", "binned");
    slice->execute();

    Workspace_sptr binned_ws =
        AnalysisDataService::Instance().retrieve("binned");
    FakeProgressAction progressUpdater;

    vtkMDHexFactory factory(VATES::VolumeNormalization);
    factory.setCheckDimensionality(doCheckDimensionality);
    if (doCheckDimensionality) {
      TS_ASSERT_THROWS(factory.initialize(binned_ws), const std::runtime_error &);
    } else {
      TS_ASSERT_THROWS_NOTHING(factory.initialize(binned_ws));
      vtkSmartPointer<vtkDataSet> product;
      TS_ASSERT_THROWS_NOTHING(product = factory.create(progressUpdater));
    }
  }

public:
  /* Destructive tests. Test works correctly when misused.*/

  void testCreateWithoutInitalizeThrows() {
    FakeProgressAction progressUpdater;
    vtkMDHexFactory factory(VATES::VolumeNormalization);
    TSM_ASSERT_THROWS("Have NOT initalized object. Should throw.",
                      factory.create(progressUpdater), const std::runtime_error &);
  }

  void testInitalizeWithNullWorkspaceThrows() {
    vtkMDHexFactory factory(VATES::VolumeNormalization);

    IMDEventWorkspace *ws = nullptr;
    TSM_ASSERT_THROWS("This is a NULL workspace. Should throw.",
                      factory.initialize(Workspace_sptr(ws)),
                      const std::invalid_argument &);
  }

  void testGetFactoryTypeName() {
    vtkMDHexFactory factory(VATES::VolumeNormalization);
    TS_ASSERT_EQUALS("vtkMDHexFactory", factory.getFactoryTypeName());
  }

  void testInitializeDelegatesToSuccessor() {
    auto mockSuccessor = new MockvtkDataSetFactory();
    auto uniqueSuccessor =
        std::unique_ptr<MockvtkDataSetFactory>(mockSuccessor);
    EXPECT_CALL(*mockSuccessor, initialize(_)).Times(1);
    EXPECT_CALL(*mockSuccessor, getFactoryTypeName()).Times(1);

    vtkMDHexFactory factory(VATES::VolumeNormalization);
    factory.setSuccessor(std::move(uniqueSuccessor));

    auto ws = boost::make_shared<Mantid::DataObjects::TableWorkspace>();
    TS_ASSERT_THROWS_NOTHING(factory.initialize(ws));

    // Need the raw pointer to test assertions here. Object is not yet deleted
    // as the factory is still in scope.
    TSM_ASSERT("Successor has not been used properly.",
               Mock::VerifyAndClearExpectations(mockSuccessor));
  }

  void testCreateDelegatesToSuccessor() {
    FakeProgressAction progressUpdater;
    auto mockSuccessor = new MockvtkDataSetFactory();
    auto uniqueSuccessor =
        std::unique_ptr<MockvtkDataSetFactory>(mockSuccessor);
    EXPECT_CALL(*mockSuccessor, initialize(_)).Times(1);
    EXPECT_CALL(*mockSuccessor, create(Ref(progressUpdater)))
        .Times(1)
        .WillOnce(Return(vtkSmartPointer<vtkStructuredGrid>::New()));
    EXPECT_CALL(*mockSuccessor, getFactoryTypeName()).Times(1);

    vtkMDHexFactory factory(VATES::VolumeNormalization);
    factory.setSuccessor(std::move(uniqueSuccessor));

    auto ws = boost::make_shared<Mantid::DataObjects::TableWorkspace>();
    TS_ASSERT_THROWS_NOTHING(factory.initialize(ws));
    TS_ASSERT_THROWS_NOTHING(factory.create(progressUpdater));

    TSM_ASSERT("Successor has not been used properly.",
               Mock::VerifyAndClearExpectations(mockSuccessor));
  }

  void testOnInitaliseCannotDelegateToSuccessor() {
    FakeProgressAction progressUpdater;
    vtkMDHexFactory factory(VATES::VolumeNormalization);
    // factory.SetSuccessor(mockSuccessor); No Successor set.

    auto ws = boost::make_shared<Mantid::DataObjects::TableWorkspace>();
    TS_ASSERT_THROWS(factory.initialize(ws), const std::runtime_error &);
  }

  void testCreateWithoutInitializeThrows() {
    FakeProgressAction progressUpdater;
    vtkMDHexFactory factory(VATES::VolumeNormalization);
    // initialize not called!
    TS_ASSERT_THROWS(factory.create(progressUpdater), const std::runtime_error &);
  }

  void test_roundUp_positive_numbers() {
    TS_ASSERT_DELTA(roundUp(coord_t(3.7), coord_t(1.0)), 4.0, 1e-5);
    TS_ASSERT_DELTA(roundUp(coord_t(3.7), coord_t(7.1)), 7.1, 1e-5);
    TS_ASSERT_DELTA(roundUp(coord_t(7.1), coord_t(7.1)), 14.2, 1e-5);
    TS_ASSERT_DELTA(roundUp(coord_t(0.0), coord_t(3.1)), 3.1, 1e-5);
  }

  void test_roundUp_negative_numbers() {
    TS_ASSERT_DELTA(roundUp(coord_t(-0.5), coord_t(3.1)), 0.0, 1e-5);
    TS_ASSERT_DELTA(roundUp(coord_t(-4.1), coord_t(3.1)), -3.1, 1e-5);
    TS_ASSERT_DELTA(roundUp(coord_t(-4.1), coord_t(1.0)), -4.0, 1e-5);
  }

  void test_roundDown_positive_numbers() {
    TS_ASSERT_DELTA(roundDown(coord_t(3.7), coord_t(1.0)), 3.0, 1e-5);
    TS_ASSERT_DELTA(roundDown(coord_t(3.7), coord_t(7.1)), 0.0, 1e-5);
    TS_ASSERT_DELTA(roundDown(coord_t(7.1), coord_t(7.1)), 7.1, 1e-5);
  }

  void test_roundDown_negative_numbers() {
    TS_ASSERT_DELTA(roundDown(coord_t(-0.5), coord_t(3.1)), -3.1, 1e-5);
    TS_ASSERT_DELTA(roundDown(coord_t(-4.1), coord_t(3.1)), -6.2, 1e-5);
    TS_ASSERT_DELTA(roundDown(coord_t(-4.1), coord_t(1.0)), -5.0, 1e-5);
    TS_ASSERT_DELTA(roundDown(coord_t(-4.0), coord_t(1.0)), -4.0, 1e-5);
  }

  /*Demonstrative tests*/
  void testIgnoresDimensionality() { doDimensionalityTesting(true); }

  void testDoNotIgnoreDimensionality() { doDimensionalityTesting(false); }

  void test_3DWorkspace() {
    FakeProgressAction progressUpdate;

    Mantid::DataObjects::MDEventWorkspace3Lean::sptr ws =
        MDEventsTestHelper::makeMDEW<3>(10, 0.0, 10.0, 1);
    vtkMDHexFactory factory(VATES::VolumeNormalization);
    factory.initialize(ws);
    vtkSmartPointer<vtkDataSet> product;

    TS_ASSERT_THROWS_NOTHING(product = factory.create(progressUpdate));

    const size_t expected_n_points = 8000;
    const size_t expected_n_cells = 1000;
    const size_t expected_n_signals = expected_n_cells;

    TSM_ASSERT_EQUALS("Wrong number of points", expected_n_points,
                      product->GetNumberOfPoints());
    TSM_ASSERT_EQUALS("Wrong number of cells", expected_n_cells,
                      product->GetNumberOfCells());
    TSM_ASSERT_EQUALS(
        "Wrong number of points to cells. Hexahedron has 8 vertexes.",
        expected_n_cells * 8, product->GetNumberOfPoints());
    TSM_ASSERT_EQUALS(
        "No signal Array", "signal",
        std::string(product->GetCellData()->GetArray(0)->GetName()));
    TSM_ASSERT_EQUALS("Wrong sized signal Array", expected_n_signals,
                      product->GetCellData()->GetArray(0)->GetSize());

    /*Check dataset bounds*/
    double *bounds = product->GetBounds();
    TS_ASSERT_EQUALS(0, bounds[0]);
    TS_ASSERT_EQUALS(10, bounds[1]);
    TS_ASSERT_EQUALS(0, bounds[2]);
    TS_ASSERT_EQUALS(10, bounds[3]);
    TS_ASSERT_EQUALS(0, bounds[4]);
    TS_ASSERT_EQUALS(10, bounds[5]);
  }

  void test_4DWorkspace() {
    MockProgressAction mockProgressAction;
    EXPECT_CALL(mockProgressAction, eventRaised(_)).Times(AtLeast(1));

    Mantid::DataObjects::MDEventWorkspace4Lean::sptr ws =
        MDEventsTestHelper::makeMDEW<4>(5, -10.0, 10.0, 1);
    vtkMDHexFactory factory(VATES::VolumeNormalization);
    factory.initialize(ws);
    vtkSmartPointer<vtkDataSet> product;

    TS_ASSERT_THROWS_NOTHING(product = factory.create(mockProgressAction));

    const size_t expected_n_points = 8 * 125;
    const size_t expected_n_cells = 125;
    const size_t expected_n_signals = expected_n_cells;

    TSM_ASSERT_EQUALS("Wrong number of points", expected_n_points,
                      product->GetNumberOfPoints());
    TSM_ASSERT_EQUALS("Wrong number of cells", expected_n_cells,
                      product->GetNumberOfCells());
    TSM_ASSERT_EQUALS(
        "Wrong number of points to cells. Hexahedron has 8 vertexes.",
        expected_n_cells * 8, product->GetNumberOfPoints());
    TSM_ASSERT_EQUALS(
        "No signal Array", "signal",
        std::string(product->GetCellData()->GetArray(0)->GetName()));
    TSM_ASSERT_EQUALS("Wrong sized signal Array", expected_n_signals,
                      product->GetCellData()->GetArray(0)->GetSize());

    /*Check dataset bounds*/
    double *bounds = product->GetBounds();
    TS_ASSERT_EQUALS(-10.0, bounds[0]);
    TS_ASSERT_EQUALS(10, bounds[1]);
    TS_ASSERT_EQUALS(-10, bounds[2]);
    TS_ASSERT_EQUALS(10, bounds[3]);
    TS_ASSERT_EQUALS(-10, bounds[4]);
    TS_ASSERT_EQUALS(10, bounds[5]);

    TSM_ASSERT("Progress reporting has not been conducted as expected",
               Mock::VerifyAndClearExpectations(&mockProgressAction));
  }

  void test_4DWorkspace_slice_on_boundary() {
    // Regression test to check nothing fails when the 4D workspace is sliced to
    // 3D on a bin boundary
    // The slice is taken at 0 in the 4th dimension and falls on a bin boundary
    // because we have an even number of bins

    MockProgressAction mockProgressAction;
    EXPECT_CALL(mockProgressAction, eventRaised(_)).Times(AtLeast(1));

    Mantid::DataObjects::MDEventWorkspace4Lean::sptr ws =
        MDEventsTestHelper::makeMDEW<4>(4, -10.0, 10.0, 1);
    vtkMDHexFactory factory(VATES::VolumeNormalization);
    factory.initialize(ws);
    vtkSmartPointer<vtkDataSet> product;

    TS_ASSERT_THROWS_NOTHING(product = factory.create(mockProgressAction));

    const size_t expected_n_points = 8 * 64;
    const size_t expected_n_cells = 64;
    const size_t expected_n_signals = expected_n_cells;

    TSM_ASSERT_EQUALS("Wrong number of points", expected_n_points,
                      product->GetNumberOfPoints());
    TSM_ASSERT_EQUALS("Wrong number of cells", expected_n_cells,
                      product->GetNumberOfCells());
    TSM_ASSERT_EQUALS(
        "Wrong number of points to cells. Hexahedron has 8 vertexes.",
        expected_n_cells * 8, product->GetNumberOfPoints());
    TSM_ASSERT_EQUALS(
        "No signal Array", "signal",
        std::string(product->GetCellData()->GetArray(0)->GetName()));
    TSM_ASSERT_EQUALS("Wrong sized signal Array", expected_n_signals,
                      product->GetCellData()->GetArray(0)->GetSize());
  }
};

//=====================================================================================
// Performance tests
//=====================================================================================
class vtkMDHexFactoryTestPerformance : public CxxTest::TestSuite {

private:
  Mantid::DataObjects::MDEventWorkspace3Lean::sptr m_ws3;
  Mantid::DataObjects::MDEventWorkspace4Lean::sptr m_ws4;

public:
  void setUp() override {
    m_ws3 = MDEventsTestHelper::makeMDEW<3>(100, 0.0, 100.0, 1);
    m_ws4 = MDEventsTestHelper::makeMDEW<4>(32, -50.0, 50.0, 1);
  }

  /* Create 1E6 cells*/
  void test_CreateDataSet_from3D() {
    FakeProgressAction progressUpdate;

    vtkMDHexFactory factory(VATES::VolumeNormalization);
    factory.initialize(m_ws3);
    vtkSmartPointer<vtkDataSet> product;

    TS_ASSERT_THROWS_NOTHING(product = factory.create(progressUpdate));

    const size_t expected_n_points = 8000000;
    const size_t expected_n_cells = 1000000;
    const size_t expected_n_signals = expected_n_cells;

    TSM_ASSERT_EQUALS("Wrong number of points", expected_n_points,
                      product->GetNumberOfPoints());
    TSM_ASSERT_EQUALS("Wrong number of cells", expected_n_cells,
                      product->GetNumberOfCells());
    TSM_ASSERT_EQUALS(
        "Wrong number of points to cells. Hexahedron has 8 vertexes.",
        expected_n_cells * 8, product->GetNumberOfPoints());
    TSM_ASSERT_EQUALS(
        "No signal Array", "signal",
        std::string(product->GetCellData()->GetArray(0)->GetName()));
    TSM_ASSERT_EQUALS("Wrong sized signal Array", expected_n_signals,
                      product->GetCellData()->GetArray(0)->GetSize());

    if (false) {
      /*Check dataset bounds - this call takes a significant amount of time and
       * so should only be used for debugging.*/
      double *bounds = product->GetBounds();
      TS_ASSERT_EQUALS(0, bounds[0]);
      TS_ASSERT_EQUALS(100, bounds[1]);
      TS_ASSERT_EQUALS(0, bounds[2]);
      TS_ASSERT_EQUALS(100, bounds[3]);
      TS_ASSERT_EQUALS(0, bounds[4]);
      TS_ASSERT_EQUALS(100, bounds[5]);
    }
  }
  /* Create 1E6 cells*/
  void test_CreateDataSet_from4D() {
    FakeProgressAction progressUpdate;

    vtkMDHexFactory factory(VATES::VolumeNormalization);
    factory.initialize(m_ws4);
    vtkSmartPointer<vtkDataSet> product;

    TS_ASSERT_THROWS_NOTHING(product = factory.create(progressUpdate));

    // Slice expected to be 1 bin thick (4 boxes thick in this case)
    const size_t expected_n_points = 8 * 4 * 32768;
    const size_t expected_n_cells = 4 * 32768;
    const size_t expected_n_signals = expected_n_cells;

    TSM_ASSERT_EQUALS("Wrong number of points", expected_n_points,
                      product->GetNumberOfPoints());
    TSM_ASSERT_EQUALS("Wrong number of cells", expected_n_cells,
                      product->GetNumberOfCells());
    TSM_ASSERT_EQUALS(
        "Wrong number of points to cells. Hexahedron has 8 vertexes.",
        expected_n_cells * 8, product->GetNumberOfPoints());
    TSM_ASSERT_EQUALS(
        "No signal Array", "signal",
        std::string(product->GetCellData()->GetArray(0)->GetName()));
    TSM_ASSERT_EQUALS("Wrong sized signal Array", expected_n_signals,
                      product->GetCellData()->GetArray(0)->GetSize());
  }
};

#endif
