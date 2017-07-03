#ifndef MANTIDQT_API_NONORTHOGONALTEST_H_
#define MANTIDQT_API_NONORTHOGONALTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/CoordTransformAffine.h"
#include "MantidGeometry/MDGeometry/HKL.h"
#include "MantidGeometry/MDGeometry/QSample.h"
#include "MantidKernel/UnitLabelTypes.h"
#include "MantidQtAPI/NonOrthogonal.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

class NonOrthogonalTest : public CxxTest::TestSuite {

public:
  static NonOrthogonalTest *createSuite() { return new NonOrthogonalTest; }
  static void destroySuite(NonOrthogonalTest *suite) { delete suite; }

  NonOrthogonalTest() { Mantid::API::FrameworkManager::Instance(); }

  void test_provideSkewMatrixWithOrthogonal() {
    // Arrange
    auto eventWorkspace = getOrthogonalEventWorkspace();
    Mantid::Kernel::DblMatrix skewMatrix;

    // Act
    bool orthogonalWorkspaceFailed = false;
    try {
      MantidQt::API::provideSkewMatrix(skewMatrix, *eventWorkspace);
    } catch (std::invalid_argument &) {
      orthogonalWorkspaceFailed = true;
    }
    // Assert
    TSM_ASSERT("Orthogonal workspaces should not be given a skew matrix",
               orthogonalWorkspaceFailed);
  }

  void test_provideSkewMatrixWithOrthogonalAndHKL() {
    // Arrange
    auto eventWorkspace = getOrthogonalHKLEventWorkspace();
    Mantid::Kernel::DblMatrix skewMatrix;

    // Act
    bool orthogonalWorkspaceFailed = false;
    try {
      MantidQt::API::provideSkewMatrix(skewMatrix, *eventWorkspace);
    } catch (std::invalid_argument &) {
      orthogonalWorkspaceFailed = true;
    }
    // Assert
    TSM_ASSERT("Orthogonal HKL workspaces should not be given a skew matrix",
               orthogonalWorkspaceFailed);
  }

  void test_provideSkewMatrixWithNonOrthogonal() {
    auto eventWorkspace = getNonOrthogonalEventWorkspace();
    auto exampleSkewMatrix = getExampleSkewMatrix();
    bool nonOrthogonalWorkspaceFailed = true;
    Mantid::Kernel::DblMatrix skewMatrix(3, 3, true);
    MantidQt::API::provideSkewMatrix(skewMatrix, *eventWorkspace);

    const auto numberOfColumns = skewMatrix.numCols();
    const auto numberOfRows = skewMatrix.numRows();
    for (size_t column = 0; column < numberOfColumns; ++column) {
      for (size_t row = 0; row < numberOfRows; ++row) {
        if (!skewWithinTolerance(skewMatrix[row][column],
                                 exampleSkewMatrix[row][column])) {
          nonOrthogonalWorkspaceFailed = false;
        }
      }
    }

    TSM_ASSERT("Skew matrix for nonOrthogonal workspace incorrect",
               nonOrthogonalWorkspaceFailed);
  }

  void test_requiresSkewMatrixWithOrthogonal() {
    auto eventWorkspace = getOrthogonalEventWorkspace();
    TSM_ASSERT("Orthogonal workspaces should not require a skew matrix",
               !MantidQt::API::requiresSkewMatrix(*eventWorkspace));
  }

  void test_requiresSkewMatrixWithOrthogonalandHKL() {
    auto eventWorkspace = getOrthogonalHKLEventWorkspace();
    TSM_ASSERT("Orthogonal HKL workspaces should not require a skew matrix",
               !MantidQt::API::requiresSkewMatrix(*eventWorkspace));
  }

  void test_requiresSkewMatrixWithNonOrthogonal() {
    auto eventWorkspace = getNonOrthogonalEventWorkspace();
    TSM_ASSERT("NonOrthogonal workspaces should require a skew matrix",
               MantidQt::API::requiresSkewMatrix(*eventWorkspace));
  }

  void test_isHKLDimensionsWithOrthogonal() {
    auto eventWorkspace = getOrthogonalEventWorkspace();
    TSM_ASSERT("Should not have HKL dimensions",
               !MantidQt::API::isHKLDimensions(*eventWorkspace, DimX, DimY));
  }

  void test_isHKLDimensionsWithOrthogonalandHKL() {
    auto eventWorkspace = getOrthogonalHKLEventWorkspace();
    TSM_ASSERT("Should have HKL dimensions",
               MantidQt::API::isHKLDimensions(*eventWorkspace, DimX, DimY));
  }

  void test_isHKLDimensionsWithNonOrthogonal() {
    auto eventWorkspace = getNonOrthogonalEventWorkspace();
    TSM_ASSERT("Should have HKL dimensions",
               MantidQt::API::isHKLDimensions(*eventWorkspace, DimX, DimY));
  }

  void test_getGridLineAnglesInRadianWithZeroArray() {
    std::array<Mantid::coord_t, 9> skewMatrixCoord = {{}};
    auto radianAngles =
        MantidQt::API::getGridLineAnglesInRadian(skewMatrixCoord, DimX, DimY);
    TSM_ASSERT("When given Zero array, getGridLinesInRadian should return "
               "0 for result.first",
               radianAngles.first == 0);
    TSM_ASSERT("When given Zero array, getGridLinesInRadian should return "
               "Nan for result,second",
               std::isnan(radianAngles.second));
  }

  void test_getGridLineAnglesInRadianWithDefaultTestArray() {
    auto skewMatrixCoord = getExampleCoordTArray(true);
    auto radianAngles =
        MantidQt::API::getGridLineAnglesInRadian(skewMatrixCoord, DimX, DimY);
    TSM_ASSERT("When given default array, getGridLinesInRadian should return 0 "
               "for result.first",
               radianAngles.first == 0);
    TSM_ASSERT("When given Zero array, getGridLinesInRadian should return 0 "
               "for result.second",
               radianAngles.second == 0);
  }

  void test_transformLookpointToWorkspaceCoordWithZeroZeroCoords() {
    auto skewMatrixCoord = getExampleCoordTArray(true);
    auto eventWorkspace = getOrthogonalHKLEventWorkspace();
    Mantid::Kernel::VMD coords = eventWorkspace->getNumDims();
    for (size_t d = 0; d < eventWorkspace->getNumDims(); d++) {
      coords[d] = Mantid::Kernel::VMD_t(0.0);
    }
    MantidQt::API::transformLookpointToWorkspaceCoord(coords, skewMatrixCoord,
                                                      DimX, DimY, SliceDim);
    TSM_ASSERT("coords[0]=0 should not be affected by skewMatrix "
               "translation",
               (coords[0] == 0));
    TSM_ASSERT("coords[0]=0 should not be affected by skewMatrix "
               "translation",
               (coords[1] == 0));
  }

  void test_transformLookPointToWorkspaceCoordWithExampleCoords() {
    auto skewMatrixCoord = getExampleCoordTArray(false);
    auto eventWorkspace = getNonOrthogonalEventWorkspace();
    Mantid::Kernel::VMD coords = eventWorkspace->getNumDims();
    for (size_t d = 0; d < eventWorkspace->getNumDims(); d++) {
      coords[d] = Mantid::Kernel::VMD_t(1.5);
    }
    MantidQt::API::transformLookpointToWorkspaceCoord(coords, skewMatrixCoord,
                                                      DimX, DimY, SliceDim);
    bool coordsAccurate(false);
    if ((skewWithinTolerance(coords[0],
                             static_cast<Mantid::Kernel::VMD_t>(0.75))) &&
        (coords[1] == 1.5) && (coords[2] == 1.5) && (coords[3] == 1.5)) {
      coordsAccurate = true;
    }
    TSM_ASSERT("Example coordinates skewed result incorrect", coordsAccurate);
  }

private:
  Mantid::API::IMDEventWorkspace_sptr getOrthogonalHKLEventWorkspace() {
    Mantid::Kernel::ReciprocalLatticeUnitFactory factory;
    auto product = factory.create(Mantid::Kernel::Units::Symbol::RLU);
    Mantid::Geometry::HKL frame(product);
    auto workspace =
        Mantid::DataObjects::MDEventsTestHelper::makeMDEWWithFrames<3>(
            5, -10, 10, frame);
    return workspace;
  }

  Mantid::DataObjects::EventWorkspace_sptr getOrthogonalEventWorkspace() {
    Mantid::DataObjects::EventWorkspace_sptr ws =
        WorkspaceCreationHelper::createEventWorkspace();
    return ws;
  }

  Mantid::API::IMDEventWorkspace_sptr getNonOrthogonalEventWorkspace(
      bool wrongCoords = false, bool forgetUB = false, bool forgetWmat = false,
      bool forgetAffmat = false, double scale = 1.0) {
    Mantid::API::IMDEventWorkspace_sptr ws;
    std::string wsName = "simpleWS";
    if (wrongCoords) {
      Mantid::Geometry::QSample frame;
      ws = Mantid::DataObjects::MDEventsTestHelper::makeAnyMDEWWithFrames<
          Mantid::DataObjects::MDEvent<4>, 4>(1, 0.0, 1.0, frame, 1, wsName);
    } else {
      Mantid::Kernel::ReciprocalLatticeUnitFactory factory;
      auto product = factory.create(Mantid::Kernel::Units::Symbol::RLU);
      Mantid::Geometry::HKL frame(product);
      ws = Mantid::DataObjects::MDEventsTestHelper::makeAnyMDEWWithFrames<
          Mantid::DataObjects::MDEvent<4>, 4>(1, 0.0, 1.0, frame, 1, wsName);
    }
    if (!forgetUB) {
      // add UB Matrix
      Mantid::API::IAlgorithm_sptr alg =
          Mantid::API::AlgorithmManager::Instance().create("SetUB");
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

    Mantid::DataObjects::CoordTransformAffine affMat(4, 4);
    affMat.setMatrix(Mantid::Kernel::Matrix<Mantid::coord_t>(affMatVals));
    if (!forgetAffmat) {
      ws->setTransformToOriginal(affMat.clone(), 0);
    }
    // Create the transform (W) matrix
    // Need it as a vector
    std::vector<double> wMat;
    Mantid::Kernel::DblMatrix temp(3, 3, true);
    wMat = temp.getVector();
    if (!forgetWmat) {
      auto p = new Mantid::Kernel::PropertyWithValue<std::vector<double>>(
          "W_MATRIX", wMat);
      ws->getExperimentInfo(0)->mutableRun().addProperty(p, true);
    }
    return ws;
  }

  Mantid::Kernel::DblMatrix getExampleSkewMatrix() {
    Mantid::Kernel::DblMatrix skewMatrix(3, 3, true);
    skewMatrix[0][0] = 1;
    skewMatrix[0][1] = 0;
    skewMatrix[0][2] = -0.57735;
    skewMatrix[1][0] = 0;
    skewMatrix[1][1] = 1;
    skewMatrix[1][2] = 0;
    skewMatrix[2][0] = 0;
    skewMatrix[2][1] = 0;
    skewMatrix[2][2] = 1.1547;
    return skewMatrix;
  }

  template <typename T> bool skewWithinTolerance(T coord, T target) {
    return (coord >= target - 0.000005) && (coord <= target + 0.000005);
  }

  std::array<Mantid::coord_t, 9> getExampleCoordTArray(bool nonSkewed) {
    std::array<Mantid::coord_t, 9> coordinates;
    if (nonSkewed) {
      coordinates = {{1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0}};
    } else {
      coordinates = {{1.0, 0.0, static_cast<Mantid::coord_t>(-0.57735), 0.0,
                      1.0, 0.0, 0.0, 0.0,
                      static_cast<Mantid::coord_t>(1.1547)}};
    }
    return coordinates;
  }

  enum DimensionIndex { DimX = 0, DimY = 1, SliceDim = 2 };
};

#endif /* MANTIDQT_API_NONORTHOGONALTEST_H_ */
