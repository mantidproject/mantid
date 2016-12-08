#ifndef MANTIDQT_API_NONORTHOGONALTEST_H_
#define MANTIDQT_API_NONORTHOGONALTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidQtAPI/NonOrthogonal.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/MDUnitFactory.h"
#include "MantidGeometry/MDGeometry/HKL.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitLabelTypes.h"
#include "MantidGeometry/MDGeometry/QSample.h"
#include "MantidDataObjects/CoordTransformAffine.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/MDUnit.h"
#include "MantidCrystal/SetUB.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

class NonOrthogonalTest : public CxxTest::TestSuite {
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
  size_t m_dimX = 0;
  size_t m_dimY = 1;
  size_t m_sliceDim = 2;

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
      Mantid::Kernel::PropertyWithValue<std::vector<double>> *p;
      p = new Mantid::Kernel::PropertyWithValue<std::vector<double>>("W_MATRIX",
                                                                     wMat);
      ws->getExperimentInfo(0)->mutableRun().addProperty(p, true);
    }
    return ws;
  }

  Mantid::Kernel::DblMatrix getExampleSkewMatrix() {
    Mantid::Kernel::DblMatrix skewMatrix(3, 3, true);
    skewMatrix[0][0] = 1; // 1
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
    if ((coord >= target - 0.000005) && (coord <= target + 0.000005)) {
      return true;
    } else
      return false;
  }

  void getExampleCoordTArray(Mantid::coord_t *coordTArrayExample,
                             bool nonSkewed) {
    if (nonSkewed) {
      coordTArrayExample[0] = 1.0;
      coordTArrayExample[1] = 0.0;
      coordTArrayExample[2] = 0.0;
      coordTArrayExample[3] = 0.0;
      coordTArrayExample[4] = 1.0;
      coordTArrayExample[5] = 0.0;
      coordTArrayExample[6] = 0.0;
      coordTArrayExample[7] = 0.0;
      coordTArrayExample[8] = 1.0;
    } else {
      coordTArrayExample[0] = 1.0;
      coordTArrayExample[1] = 0.0;
      coordTArrayExample[2] = static_cast<Mantid::coord_t>(-0.57735);
      coordTArrayExample[3] = 0.0;
      coordTArrayExample[4] = 1.0;
      coordTArrayExample[5] = 0.0;
      coordTArrayExample[6] = 0.0;
      coordTArrayExample[7] = 0.0;
      coordTArrayExample[8] = static_cast<Mantid::coord_t>(1.1547);
    }
  }

public:
  static NonOrthogonalTest *createSuite() {
    Mantid::API::FrameworkManager::Instance();
    Mantid::API::AlgorithmManager::Instance();
    return new NonOrthogonalTest;
  }
  static void destroySuite(NonOrthogonalTest *suite) { delete suite; }

  void test_provideSkewMatrixWithOrthogonal() {
    // Arrange
    auto eventWorkspace = getOrthogonalEventWorkspace();
    Mantid::Kernel::DblMatrix skewMatrix;

    // Act
    bool orthogonalWorkspaceFailed = false;
    try {
      MantidQt::API::provideSkewMatrix(skewMatrix, eventWorkspace);
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
      MantidQt::API::provideSkewMatrix(skewMatrix, eventWorkspace);
    } catch (std::invalid_argument &) {
      orthogonalWorkspaceFailed = true;
    }
    // Assert
    TSM_ASSERT("Orthogonal HKL workspaces should not be given a skew matrix",
               orthogonalWorkspaceFailed);
  }

  void test_provideSkewMatrixWithNonOrthogonal() {
    auto eventWorkspace = getNonOrthogonalEventWorkspace();
    Mantid::Kernel::DblMatrix skewMatrix(3, 3, true);
    Mantid::Kernel::DblMatrix exampleSkewMatrix(3, 3, true);
    exampleSkewMatrix = getExampleSkewMatrix();
    bool nonOrthogonalWorkspaceFailed = true;

    MantidQt::API::provideSkewMatrix(skewMatrix, eventWorkspace);

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
    bool requiresSkewMatrix;
    requiresSkewMatrix = MantidQt::API::requiresSkewMatrix(eventWorkspace);
    TSM_ASSERT("Orthogonal workspaces should not require a skew matrix",
               !requiresSkewMatrix);
  }

  void test_requiresSkewMatrixWithOrthogonalandHKL() {
    auto eventWorkspace = getOrthogonalHKLEventWorkspace();
    bool requiresSkewMatrix;
    requiresSkewMatrix = MantidQt::API::requiresSkewMatrix(eventWorkspace);
    TSM_ASSERT("Orthogonal HKL workspaces should not require a skew matrix",
               !requiresSkewMatrix);
  }

  void test_requiresSkewMatrixWithNonOrthogonal() {
    auto eventWorkspace = getNonOrthogonalEventWorkspace();
    bool requiresSkewMatrix;
    requiresSkewMatrix = MantidQt::API::requiresSkewMatrix(eventWorkspace);
    TSM_ASSERT("NonOrthogonal workspaces should require a skew matrix",
               requiresSkewMatrix);
  }

  void test_isHKLDimensionsWithOrthogonal() {
    auto eventWorkspace = getOrthogonalEventWorkspace();
    bool hasHKLDimension;
    hasHKLDimension =
        MantidQt::API::isHKLDimensions(eventWorkspace, m_dimX, m_dimY);
    TSM_ASSERT("Should not have HKL dimensions", !hasHKLDimension);
  }

  void test_isHKLDimensionsWithOrthogonalandHKL() {
    auto eventWorkspace = getOrthogonalHKLEventWorkspace();
    bool hasHKLDimension;
    hasHKLDimension =
        MantidQt::API::isHKLDimensions(eventWorkspace, m_dimX, m_dimY);
    TSM_ASSERT("Should have HKL dimensions", hasHKLDimension);
  }

  void test_isHKLDimensionsWithNonOrthogonal() {
    auto eventWorkspace = getNonOrthogonalEventWorkspace();
    bool hasHKLDimension;
    hasHKLDimension =
        MantidQt::API::isHKLDimensions(eventWorkspace, m_dimX, m_dimY);
    TSM_ASSERT("Should have HKL dimensions", hasHKLDimension);
  }

  void test_getGridLineAnglesInRadianWithZeroArray() {
    Mantid::coord_t skewMatrixCoord[9] = {0};
    std::pair<double, double> radianAngles;
    bool radianResultCorrect;
    radianResultCorrect = false;
    radianAngles = MantidQt::API::getGridLineAnglesInRadian(skewMatrixCoord,
                                                            m_dimX, m_dimY);
    if ((radianAngles.first == 0) && (std::isnan(radianAngles.second))) {
      radianResultCorrect = true;
    }
    TSM_ASSERT("When given Zero array, getGridLinesInRadian should return 0 "
               "and Nan result",
               radianResultCorrect);
  }

  void test_getGridLineAnglesInRadianWithDefaultTestArray() {
    Mantid::coord_t skewMatrixCoord[9];
    getExampleCoordTArray(skewMatrixCoord, true);
    std::pair<double, double> radianAngles;
    bool radianResultCorrect;
    radianResultCorrect = false;
    radianAngles = MantidQt::API::getGridLineAnglesInRadian(skewMatrixCoord,
                                                            m_dimX, m_dimY);
    if ((radianAngles.first == 0) && (radianAngles.second == 0)) {
      radianResultCorrect = true;
    }
    TSM_ASSERT("When given default array, getGridLinesInRadian should return 0 "
               "and 0 result",
               radianResultCorrect);
  }

  void test_transformLookpointToWorkspaceCoordWithZeroZeroCoords() {
    bool coordsRemainZero;
    coordsRemainZero = false;
    Mantid::coord_t skewMatrixCoord[9];
    getExampleCoordTArray(skewMatrixCoord, true);
    auto eventWorkspace = getOrthogonalEventWorkspace();
    Mantid::Kernel::VMD coords = eventWorkspace->getNumDims();
    for (size_t d = 0; d < eventWorkspace->getNumDims();
         d++) // change to num dims of eventworkspace
    {
      coords[d] = Mantid::Kernel::VMD_t(0.0);
    }
    MantidQt::API::transformLookpointToWorkspaceCoordGeneric(
        coords, skewMatrixCoord, m_dimX, m_dimY, m_sliceDim);
    if ((coords[0] == 0) && (coords[1] == 0)) {
      coordsRemainZero = true;
    }
    TSM_ASSERT("Zero, zero coordinates should not be affected by skewMatrix "
               "translation",
               coordsRemainZero);
  }

  void test_transformLookPointToWorkspaceCoordWithExampleCoords() {
    bool coordsAccurate;
    coordsAccurate = false;
    Mantid::coord_t skewMatrixCoord[9];
    getExampleCoordTArray(skewMatrixCoord, false);
    auto eventWorkspace = getNonOrthogonalEventWorkspace();
    Mantid::Kernel::VMD coords = eventWorkspace->getNumDims();
    for (size_t d = 0; d < eventWorkspace->getNumDims(); d++) {
      coords[d] = Mantid::Kernel::VMD_t(1.5);
    }
    MantidQt::API::transformLookpointToWorkspaceCoordGeneric(
        coords, skewMatrixCoord, m_dimX, m_dimY, m_sliceDim);
    if ((skewWithinTolerance(coords[0],
                             static_cast<Mantid::Kernel::VMD_t>(0.75))) &&
        (coords[1] == 1.5) && (coords[2] == 1.5) && (coords[3] == 1.5)) {
      coordsAccurate = true;
    }
    TSM_ASSERT("Example coordinates skewed result incorrect", coordsAccurate);
  }
};

#endif /* MANTIDQT_API_NONORTHOGONALTEST_H_ */
