// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_VATES_VTKDATASETTONONORTHOGONALDATASETTEST_H_
#define MANTID_VATES_VTKDATASETTONONORTHOGONALDATASETTEST_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidDataObjects/CoordTransformAffine.h"
#include "MantidGeometry/MDGeometry/HKL.h"
#include "MantidGeometry/MDGeometry/QSample.h"
#include "MantidKernel/MDUnit.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidVatesAPI/vtkDataSetToNonOrthogonalDataSet.h"
#include "MantidVatesAPI/ADSWorkspaceProvider.h"
#include <cxxtest/TestSuite.h>

#include "MantidVatesAPI/vtkRectilinearGrid_Silent.h"
#include <vtkDataArray.h>
#include <vtkFieldData.h>
#include <vtkFloatArray.h>
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::DataObjects::MDEventsTestHelper;
using namespace Mantid::VATES;

class vtkDataSetToNonOrthogonalDataSetTest : public CxxTest::TestSuite {
private:
  std::string
  createMantidWorkspace(bool nonUnityTransform, bool wrongCoords = false,
                        bool forgetUB = false, bool forgetWmat = false,
                        bool forgetAffmat = false, double scale = 1.0) {
    // Creating an MDEventWorkspace as the content is not germain to the
    // information necessary for the non-orthogonal axes
    std::string wsName = "simpleWS";
    IMDEventWorkspace_sptr ws;
    // Set the coordinate system
    if (wrongCoords) {
      Mantid::Geometry::QSample frame;
      ws = MDEventsTestHelper::makeAnyMDEWWithFrames<MDEvent<4>, 4>(
          1, 0.0, 1.0, frame, 1, wsName);

    } else {
      Mantid::Geometry::HKL frame(new Mantid::Kernel::ReciprocalLatticeUnit);
      ws = MDEventsTestHelper::makeAnyMDEWWithFrames<MDEvent<4>, 4>(
          1, 0.0, 1.0, frame, 1, wsName);
    }

    // Set the UB matrix
    if (!forgetUB) {
      IAlgorithm_sptr alg = AlgorithmManager::Instance().create("SetUB");
      alg->initialize();
      alg->setRethrows(true);
      alg->setProperty("Workspace", wsName);
      alg->setProperty("a", 3.643 * scale);
      alg->setProperty("b", 3.643);
      alg->setProperty("c", 5.781);
      alg->setProperty("alpha", 90.0);
      alg->setProperty("beta", 90.0);
      alg->setProperty("gamma", 120.0);
      std::vector<double> uVec;
      uVec.push_back(1 * scale);
      uVec.push_back(1);
      uVec.push_back(0);
      std::vector<double> vVec;
      vVec.push_back(0);
      vVec.push_back(0);
      vVec.push_back(1);
      alg->setProperty("u", uVec);
      alg->setProperty("v", vVec);
      alg->execute();
    }

    // Create the coordinate transformation information
    std::vector<Mantid::coord_t> affMatVals{1, 0, 0, 0, 0, 0, 0, 1, 0,
                                            0, 0, 0, 0, 1, 0, 0, 1, 0,
                                            0, 0, 0, 0, 0, 0, 1};

    CoordTransformAffine affMat(4, 4);
    affMat.setMatrix(Matrix<Mantid::coord_t>(affMatVals));
    if (!forgetAffmat) {
      ws->setTransformToOriginal(affMat.clone(), 0);
    }

    // Create the transform (W) matrix
    // Need it as a vector
    std::vector<double> wMat;
    if (!nonUnityTransform) {
      DblMatrix temp(3, 3, true);
      wMat = temp.getVector();
    } else {
      wMat = {1, 1, 0, 1, -1, 0, 0, 0, 1};
    }

    if (!forgetWmat) {
      // Create property for W matrix and add it as log to run object
      PropertyWithValue<std::vector<double>> *p;
      p = new PropertyWithValue<std::vector<double>>("W_MATRIX", wMat);
      ws->getExperimentInfo(0)->mutableRun().addProperty(p, true);
    }

    return wsName;
  }

  vtkUnstructuredGrid *createSingleVoxelPoints() {
    vtkUnstructuredGrid *ds = vtkUnstructuredGrid::New();
    vtkPoints *points = vtkPoints::New();
    points->Allocate(8);
    points->InsertNextPoint(0, 0, 0);
    points->InsertNextPoint(1, 0, 0);
    points->InsertNextPoint(1, 1, 0);
    points->InsertNextPoint(0, 1, 0);
    points->InsertNextPoint(0, 0, 1);
    points->InsertNextPoint(1, 0, 1);
    points->InsertNextPoint(1, 1, 1);
    points->InsertNextPoint(0, 1, 1);

    ds->SetPoints(points);
    return ds;
  }

  std::vector<double> getRangeComp(vtkDataSet *ds, const char *fieldname) {
    vtkDataArray *arr = ds->GetFieldData()->GetArray(fieldname);
    std::vector<double> vals(arr->GetNumberOfComponents());
    TS_ASSERT_EQUALS(vals.size(), 16);
    arr->GetTuple(0, vals.data());
    return vals;
  }

  void checkUnityTransformation(vtkUnstructuredGrid *grid) {
    // This function can be used for both the unscaled and scaled
    // unity transformation, since the outcome is identical.
    // Now, check some values
    /// Get the (1,1,1) point
    const double eps = 1.0e-5;
    double *point = grid->GetPoint(6);
    TS_ASSERT_DELTA(point[0], 1.5, eps);
    TS_ASSERT_DELTA(point[1], 1.0, eps);
    TS_ASSERT_DELTA(point[2], 0.8660254, eps);
    // See if the basis vectors are available

    std::vector<double> basisMatrix = getRangeComp(grid, "ChangeOfBasisMatrix");

    // Row by row check

    // basisX[0], basisY[0], basisZ[0], 0
    int index = 0;
    TS_ASSERT_DELTA(basisMatrix[index++], 1.0, eps);
    TS_ASSERT_DELTA(basisMatrix[index++], 0.0, eps);
    TS_ASSERT_DELTA(basisMatrix[index++], 0.5, eps);
    TS_ASSERT_DELTA(basisMatrix[index++], 0.0, eps);

    // basisX[1], basisY[1], basisZ[1], 0
    TS_ASSERT_DELTA(basisMatrix[index++], 0.0, eps);
    TS_ASSERT_DELTA(basisMatrix[index++], 1.0, eps);
    TS_ASSERT_DELTA(basisMatrix[index++], 0.0, eps);
    TS_ASSERT_DELTA(basisMatrix[index++], 0.0, eps);

    // basisX[2], basisY[2], basisZ[2], 0
    TS_ASSERT_DELTA(basisMatrix[index++], 0.0, eps);
    TS_ASSERT_DELTA(basisMatrix[index++], 0.0, eps);
    TS_ASSERT_DELTA(basisMatrix[index++], 0.8660254, eps);
    TS_ASSERT_DELTA(basisMatrix[index++], 0.0, eps);

    // 0, 0, 0, 1
    TS_ASSERT_DELTA(basisMatrix[index++], 0.0, eps);
    TS_ASSERT_DELTA(basisMatrix[index++], 0.0, eps);
    TS_ASSERT_DELTA(basisMatrix[index++], 0.0, eps);
    TS_ASSERT_DELTA(basisMatrix[index++], 1.0, eps);
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static vtkDataSetToNonOrthogonalDataSetTest *createSuite() {
    return new vtkDataSetToNonOrthogonalDataSetTest();
  }
  static void destroySuite(vtkDataSetToNonOrthogonalDataSetTest *suite) {
    delete suite;
  }

  void testThrowIfVtkDatasetNull() {
    vtkDataSet *dataset = nullptr;
    auto workspaceProvider = std::make_unique<
        ADSWorkspaceProvider<Mantid::API::IMDWorkspace>>();
    TS_ASSERT_THROWS(vtkDataSetToNonOrthogonalDataSet temp(
                         dataset, "", std::move(workspaceProvider)),
                     const std::runtime_error &);
  }

  void testThrowsIfWorkspaceNameEmptyAndUsingADSWorkspaceProvider() {
    vtkNew<vtkUnstructuredGrid> dataset;
    auto workspaceProvider = std::make_unique<
        ADSWorkspaceProvider<Mantid::API::IMDWorkspace>>();
    TS_ASSERT_THROWS(
        vtkDataSetToNonOrthogonalDataSet temp(dataset.GetPointer(), "",
                                              std::move(workspaceProvider)),
        const std::runtime_error &);
  }

  void testThrowIfVtkDatasetWrongType() {
    vtkNew<vtkRectilinearGrid> grid;
    auto workspaceProvider = std::make_unique<
        ADSWorkspaceProvider<Mantid::API::IMDWorkspace>>();
    vtkDataSetToNonOrthogonalDataSet converter(grid.GetPointer(), "name",
                                               std::move(workspaceProvider));
    TS_ASSERT_THROWS(converter.execute(), const std::runtime_error &);
  }

  void testSimpleDataset() {
    std::string wsName = createMantidWorkspace(false);
    vtkSmartPointer<vtkUnstructuredGrid> ds;
    ds.TakeReference(createSingleVoxelPoints());
    auto workspaceProvider = std::make_unique<
        ADSWorkspaceProvider<Mantid::API::IMDWorkspace>>();
    vtkDataSetToNonOrthogonalDataSet converter(ds, wsName,
                                               std::move(workspaceProvider));
    TS_ASSERT_THROWS_NOTHING(converter.execute());
    this->checkUnityTransformation(ds);
  }

  void testThrowsSimpleDatasetWrongCoords() {
    std::string wsName = createMantidWorkspace(false, true);
    vtkSmartPointer<vtkUnstructuredGrid> ds;
    ds.TakeReference(createSingleVoxelPoints());
    auto workspaceProvider = std::make_unique<
        ADSWorkspaceProvider<Mantid::API::IMDWorkspace>>();
    vtkDataSetToNonOrthogonalDataSet converter(ds, wsName,
                                               std::move(workspaceProvider));
    TS_ASSERT_THROWS(converter.execute(), const std::invalid_argument &);
  }

  void testThrowsSimpleDatasetNoUB() {
    std::string wsName = createMantidWorkspace(false, false, true);
    vtkSmartPointer<vtkUnstructuredGrid> ds;
    ds.TakeReference(createSingleVoxelPoints());
    auto workspaceProvider = std::make_unique<
        ADSWorkspaceProvider<Mantid::API::IMDWorkspace>>();
    vtkDataSetToNonOrthogonalDataSet converter(ds, wsName,
                                               std::move(workspaceProvider));
    TS_ASSERT_THROWS(converter.execute(), const std::invalid_argument &);
  }

  void testThrowsSimpleDatasetNoWMatrix() {
    std::string wsName = createMantidWorkspace(false, false, false, true);
    vtkSmartPointer<vtkUnstructuredGrid> ds;
    ds.TakeReference(createSingleVoxelPoints());
    auto workspaceProvider = std::make_unique<
        ADSWorkspaceProvider<Mantid::API::IMDWorkspace>>();
    vtkDataSetToNonOrthogonalDataSet converter(ds, wsName,
                                               std::move(workspaceProvider));
    TS_ASSERT_THROWS(converter.execute(), const std::invalid_argument &);
  }

  void testNoThrowsSimpleDataSetNoAffineMatrix() {
    std::string wsName =
        createMantidWorkspace(false, false, false, false, true);
    vtkSmartPointer<vtkUnstructuredGrid> ds;
    ds.TakeReference(createSingleVoxelPoints());
    auto workspaceProvider = std::make_unique<
        ADSWorkspaceProvider<Mantid::API::IMDWorkspace>>();
    vtkDataSetToNonOrthogonalDataSet converter(ds, wsName,
                                               std::move(workspaceProvider));
    TS_ASSERT_THROWS_NOTHING(converter.execute());
  }

  void testNonUnitySimpleDataset() {
    std::string wsName = createMantidWorkspace(true);
    vtkSmartPointer<vtkUnstructuredGrid> ds;
    ds.TakeReference(createSingleVoxelPoints());
    auto workspaceProvider = std::make_unique<
        ADSWorkspaceProvider<Mantid::API::IMDWorkspace>>();
    vtkDataSetToNonOrthogonalDataSet converter(ds, wsName,
                                               std::move(workspaceProvider));
    TS_ASSERT_THROWS_NOTHING(converter.execute());
    // Now, check some values
    /// Get the (1,1,1) point
    const double eps = 1.0e-5;
    double *point = ds->GetPoint(6);
    TS_ASSERT_DELTA(point[0], 1.0, eps);
    TS_ASSERT_DELTA(point[1], 1.0, eps);
    TS_ASSERT_DELTA(point[2], 1.0, eps);
    // See if the basis vectors are available
    std::vector<double> basisMatrix = getRangeComp(ds, "ChangeOfBasisMatrix");

    // Row by row check

    // basisX[0], basisY[0], basisZ[0], 0
    int index = 0;
    TS_ASSERT_DELTA(basisMatrix[index++], 1.0, eps);
    TS_ASSERT_DELTA(basisMatrix[index++], 0.0, eps);
    TS_ASSERT_DELTA(basisMatrix[index++], 0.0, eps);
    TS_ASSERT_DELTA(basisMatrix[index++], 0.0, eps);

    // basisX[1], basisY[1], basisZ[1], 0
    TS_ASSERT_DELTA(basisMatrix[index++], 0.0, eps);
    TS_ASSERT_DELTA(basisMatrix[index++], 1.0, eps);
    TS_ASSERT_DELTA(basisMatrix[index++], 0.0, eps);
    TS_ASSERT_DELTA(basisMatrix[index++], 0.0, eps);

    // basisX[2], basisY[2], basisZ[2], 0
    TS_ASSERT_DELTA(basisMatrix[index++], 0.0, eps);
    TS_ASSERT_DELTA(basisMatrix[index++], 0.0, eps);
    TS_ASSERT_DELTA(basisMatrix[index++], 1.0, eps);
    TS_ASSERT_DELTA(basisMatrix[index++], 0.0, eps);

    // 0, 0, 0, 1
    TS_ASSERT_DELTA(basisMatrix[index++], 0.0, eps);
    TS_ASSERT_DELTA(basisMatrix[index++], 0.0, eps);
    TS_ASSERT_DELTA(basisMatrix[index++], 0.0, eps);
    TS_ASSERT_DELTA(basisMatrix[index++], 1.0, eps);
  }

  void testScaledSimpleDataset() {
    std::string wsName =
        createMantidWorkspace(false, false, false, false, false, 2.0);
    vtkSmartPointer<vtkUnstructuredGrid> ds;
    ds.TakeReference(createSingleVoxelPoints());
    auto workspaceProvider = std::make_unique<
        ADSWorkspaceProvider<Mantid::API::IMDWorkspace>>();
    vtkDataSetToNonOrthogonalDataSet converter(ds, wsName,
                                               std::move(workspaceProvider));
    TS_ASSERT_THROWS_NOTHING(converter.execute());
    this->checkUnityTransformation(ds);
  }

  void testScaledNonUnitySimpleDataset() {
    std::string wsName =
        createMantidWorkspace(true, false, false, false, false, 2.0);
    vtkSmartPointer<vtkUnstructuredGrid> ds;
    ds.TakeReference(createSingleVoxelPoints());
    auto workspaceProvider = std::make_unique<
        ADSWorkspaceProvider<Mantid::API::IMDWorkspace>>();
    vtkDataSetToNonOrthogonalDataSet converter(ds, wsName,
                                               std::move(workspaceProvider));
    TS_ASSERT_THROWS_NOTHING(converter.execute());
    // Now, check some values
    /// Get the (1,1,1) point
    const double eps = 1.0e-5;
    double *point = ds->GetPoint(6);
    TS_ASSERT_DELTA(point[0], 0.34534633, eps);
    TS_ASSERT_DELTA(point[1], 1.0, eps);
    TS_ASSERT_DELTA(point[2], 0.75592895, eps);
    // See if the basis vectors are available
    std::vector<double> basisMatrix = getRangeComp(ds, "ChangeOfBasisMatrix");

    // Row by row check

    // basisX[0], basisY[0], basisZ[0], 0
    int index = 0;
    TS_ASSERT_DELTA(basisMatrix[index++], 1.0, eps);
    TS_ASSERT_DELTA(basisMatrix[index++], 0.0, eps);
    TS_ASSERT_DELTA(basisMatrix[index++], -0.65465367, eps);
    TS_ASSERT_DELTA(basisMatrix[index++], 0.0, eps);

    // basisX[1], basisY[1], basisZ[1], 0
    TS_ASSERT_DELTA(basisMatrix[index++], 0.0, eps);
    TS_ASSERT_DELTA(basisMatrix[index++], 1.0, eps);
    TS_ASSERT_DELTA(basisMatrix[index++], 0.0, eps);
    TS_ASSERT_DELTA(basisMatrix[index++], 0.0, eps);

    // basisX[2], basisY[2], basisZ[2], 0
    TS_ASSERT_DELTA(basisMatrix[index++], 0.0, eps);
    TS_ASSERT_DELTA(basisMatrix[index++], 0.0, eps);
    TS_ASSERT_DELTA(basisMatrix[index++], 0.75592895, eps);
    TS_ASSERT_DELTA(basisMatrix[index++], 0.0, eps);

    // 0, 0, 0, 1
    TS_ASSERT_DELTA(basisMatrix[index++], 0.0, eps);
    TS_ASSERT_DELTA(basisMatrix[index++], 0.0, eps);
    TS_ASSERT_DELTA(basisMatrix[index++], 0.0, eps);
    TS_ASSERT_DELTA(basisMatrix[index++], 1.0, eps);
  }
};

#endif /* MANTID_VATESAPI_VTKDATASETTONONORTHOGONALDATASETTEST_H_ */
