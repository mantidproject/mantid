#ifndef MANTIDQT_SLICEVIEWER_COORDINATETRANSFORM_TEST_H
#define MANTIDQT_SLICEVIEWER_COORDINATETRANSFORM_TEST_H

#include <cxxtest/TestSuite.h>
#include "MantidAPI/FrameworkManager.h"
#include "MantidQtSliceViewer/CoordinateTransform.h"
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
#include "MantidCrystal/SetUB.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/MDUnit.h"

class CoordinateTransformTest : public CxxTest::TestSuite {
private:
  Mantid::API::IMDEventWorkspace_sptr getOrthogonalEventWorkspace() {
    Mantid::Kernel::ReciprocalLatticeUnitFactory factory;
    auto product = factory.create(Mantid::Kernel::Units::Symbol::RLU);
    Mantid::Geometry::HKL frame(product);
    auto workspace =
        Mantid::DataObjects::MDEventsTestHelper::makeMDEWWithFrames<3>(
            5, -10, 10, frame);
    return workspace;
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

  bool skewWithinTolerance(Mantid::Kernel::VMD_t coord, double target) {
    if ((coord >= target - 0.000005) && (coord <= target + 0.000005)) {
      return true;
    } else
      return false;
  }

public:
  static CoordinateTransformTest *createSuite() {
    Mantid::API::FrameworkManager::Instance();
    Mantid::API::AlgorithmManager::Instance();
    return new CoordinateTransformTest();
  }

  static void destroySuite(CoordinateTransformTest *suite) { delete suite; }

  void test_NullTransformDoesNotTransform() {
    // Arrange

    auto eventWorkspace = getOrthogonalEventWorkspace();
    Mantid::Kernel::VMD coords = eventWorkspace->getNumDims();

    // Act
    auto m_coordinateTransform =
        MantidQt::SliceViewer::createCoordinateTransform(eventWorkspace, m_dimX,
                                                         m_dimY);
    m_coordinateTransform->transform(coords, m_dimX, m_dimY, m_sliceDim);

    // cast into null
    TSM_ASSERT("Orthogonal workspaces should not be transformed",
               dynamic_cast<MantidQt::SliceViewer::NullTransform *>(
                   m_coordinateTransform.get()));
  }

  void test_NonOrthogonalTransform() {
    // Arrange
    auto eventWorkspace = getNonOrthogonalEventWorkspace();
    Mantid::Kernel::VMD coords = eventWorkspace->getNumDims();

    // Act
    auto m_coordinateTransform =
        MantidQt::SliceViewer::createCoordinateTransform(eventWorkspace, m_dimX,
                                                         m_dimY);
    m_coordinateTransform->transform(coords, m_dimX, m_dimY, m_sliceDim);

    // Assert
    TSM_ASSERT("Orthogonal workspaces should not be transformed",
               dynamic_cast<MantidQt::SliceViewer::NonOrthogonalTransform *>(
                   m_coordinateTransform.get()));
  }

  void test_NonOrthogonalZeroReturnsZero() {
    // arrange
    auto eventWorkspace = getNonOrthogonalEventWorkspace();
    Mantid::Kernel::VMD coords = eventWorkspace->getNumDims();
    for (size_t d = 0; d < eventWorkspace->getNumDims();
         d++) // change to num dims of eventworkspace
    {
      coords[d] = Mantid::Kernel::VMD_t(0.0);
    }

    // act
    auto m_coordinateTransform =
        MantidQt::SliceViewer::createCoordinateTransform(eventWorkspace, m_dimX,
                                                         m_dimY);
    m_coordinateTransform->transform(coords, m_dimX, m_dimY, m_sliceDim);

    // assert
    TSM_ASSERT_EQUALS("Zero coords should not be changed by skew matrix",
                      coords[0], 0.0);
  }

  void test_NonOrthogonalSkewCorrectness() {
    // Arrange
    auto eventWorkspace = getNonOrthogonalEventWorkspace();
    Mantid::Kernel::VMD coords = eventWorkspace->getNumDims();

    for (size_t d = 0; d < eventWorkspace->getNumDims(); d++) {
      coords[d] = Mantid::Kernel::VMD_t(1.5);
    }

    // Act
    auto m_coordinateTransform =
        MantidQt::SliceViewer::createCoordinateTransform(eventWorkspace, m_dimX,
                                                         m_dimY);
    m_coordinateTransform->transform(coords, m_dimX, m_dimY, m_sliceDim);
    double expectedValue = 0.75;
    bool skewCorrect = skewWithinTolerance(coords[0], expectedValue);
    TSM_ASSERT("Coords not skewed within limits", skewCorrect);
  }

  void test_ThrowsSimpleDatasetWrongCoords() {
    // Arrange
    auto eventWorkspace = getNonOrthogonalEventWorkspace(true);
    Mantid::Kernel::VMD coords = eventWorkspace->getNumDims();

    // Act
    auto m_coordinateTransform =
        MantidQt::SliceViewer::createCoordinateTransform(eventWorkspace, m_dimX,
                                                         m_dimY);
    m_coordinateTransform->transform(coords, m_dimX, m_dimY, m_sliceDim);
    // Assert
    TSM_ASSERT(
        "Datasets with wrong coordinates (non HKL) should not be transformed",
        dynamic_cast<MantidQt::SliceViewer::NullTransform *>(
            m_coordinateTransform.get()));
  }

  void test_ThrowsSimpleDatasetNoUBMatrix() {
    // Arrange
    auto eventWorkspace = getNonOrthogonalEventWorkspace(false, true);
    Mantid::Kernel::VMD coords = eventWorkspace->getNumDims();

    // Act
    auto m_coordinateTransform =
        MantidQt::SliceViewer::createCoordinateTransform(eventWorkspace, m_dimX,
                                                         m_dimY);
    m_coordinateTransform->transform(coords, m_dimX, m_dimY, m_sliceDim);
    // Assert
    TSM_ASSERT("Datasets without a UBmatrix should not be transformed",
               dynamic_cast<MantidQt::SliceViewer::NullTransform *>(
                   m_coordinateTransform.get()));
  }

  void test_ThrowsSimpleDatasetNoWMatrix() {
    // Arrange
    auto eventWorkspace = getNonOrthogonalEventWorkspace(false, false, true);
    Mantid::Kernel::VMD coords = eventWorkspace->getNumDims();

    // Act
    auto m_coordinateTransform =
        MantidQt::SliceViewer::createCoordinateTransform(eventWorkspace, m_dimX,
                                                         m_dimY);
    m_coordinateTransform->transform(coords, m_dimX, m_dimY, m_sliceDim);
    // Assert
    TSM_ASSERT("Datasets without a Wmatrix should not be transformed",
               dynamic_cast<MantidQt::SliceViewer::NullTransform *>(
                   m_coordinateTransform.get()));
  }

  void test_ThrowsSimpleDatasetNoAffMatrix() {
    // Arrange
    auto eventWorkspace =
        getNonOrthogonalEventWorkspace(false, false, false, true);
    Mantid::Kernel::VMD coords = eventWorkspace->getNumDims();

    // Act
    auto m_coordinateTransform =
        MantidQt::SliceViewer::createCoordinateTransform(eventWorkspace, m_dimX,
                                                         m_dimY);
    m_coordinateTransform->transform(coords, m_dimX, m_dimY, m_sliceDim);
    // Assert
    TSM_ASSERT("Datasets without a Affmatrix should still be transformed",
               dynamic_cast<MantidQt::SliceViewer::NonOrthogonalTransform *>(
                   m_coordinateTransform.get()));
  }
};
#endif
