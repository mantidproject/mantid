#include "MantidQtAPI/NonOrthogonal.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/CoordTransform.h"
#include "MantidKernel/Matrix.h"
#include "MantidGeometry/Crystal/UnitCell.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidGeometry/MDGeometry/HKL.h"

#include <cmath>
#include <boost/pointer_cast.hpp>
#include <array>
#include <math.h>
#include <algorithm>

namespace {
void checkForSampleAndRunEntries(
    const Mantid::API::Sample &sample, const Mantid::API::Run &run,
    Mantid::Kernel::SpecialCoordinateSystem specialCoordinateSystem) {

  if (Mantid::Kernel::HKL != specialCoordinateSystem) {
    throw std::invalid_argument(
        "Cannot create non-orthogonal view for non-HKL coordinates");
  }

  if (!sample.hasOrientedLattice()) {
    throw std::invalid_argument("OrientedLattice is not present on workspace");
  }

  if (!run.hasProperty("W_MATRIX")) {
    throw std::invalid_argument("W_MATRIX is not present on workspace");
  }
}

void normalizeColumns(Mantid::Kernel::DblMatrix &skewMatrix) {
  const auto numberOfColumns = skewMatrix.numCols();
  const auto numberOfRows = skewMatrix.numRows();
  std::vector<double> bNorm;
  bNorm.reserve(skewMatrix.numCols());
  for (size_t column = 0; column < numberOfColumns; ++column) {
    double sumOverRow(0.0);
    for (size_t row = 0; row < numberOfRows; ++row) {
      sumOverRow += std::pow(skewMatrix[row][column], 2);
    }
    bNorm.push_back(std::sqrt(sumOverRow));
  }

  // Apply column normalisation to skew matrix --> TODO: Check why 3 is
  // hardcoded
  const size_t dim = 3;
  Mantid::Kernel::DblMatrix scaleMat(3, 3, true);
  for (size_t index = 0; index < dim; ++index) {
    scaleMat[index][index] /= bNorm[index];
  }

  skewMatrix *= scaleMat;
}

void stripMatrix(Mantid::Kernel::DblMatrix &matrix) {
  std::size_t dim = matrix.Ssize() - 1;
  Mantid::Kernel::DblMatrix temp(dim, dim);
  for (std::size_t i = 0; i < dim; i++) {
    for (std::size_t j = 0; j < dim; j++) {
      temp[i][j] = matrix[i][j];
    }
  }
  matrix = temp;
}

template <typename T>
void doProvideSkewMatrix(Mantid::Kernel::DblMatrix &skewMatrix, T workspace) {
  // The input workspace needs have
  // 1. an HKL frame
  // 2. an oriented lattice
  // else we cannot create the skew matrix
  const auto &sample = workspace->getExperimentInfo(0)->sample();
  const auto &run = workspace->getExperimentInfo(0)->run();
  auto specialCoordnateSystem = workspace->getSpecialCoordinateSystem();
  checkForSampleAndRunEntries(sample, run, specialCoordnateSystem);

  // Create Affine Matrix
  Mantid::Kernel::Matrix<Mantid::coord_t> affineMatrix;
  try {
    auto const *transform = workspace->getTransformToOriginal();
    affineMatrix = transform->makeAffineMatrix();
  } catch (std::runtime_error &) {
    // Create identity matrix of dimension+1
    std::size_t nDims = workspace->getNumDims() + 1;
    Mantid::Kernel::Matrix<Mantid::coord_t> temp(nDims, nDims, true);
    affineMatrix = temp;
  }

  // Extract W Matrix
  auto wMatrixAsArray =
      run.template getPropertyValueAsType<std::vector<double>>("W_MATRIX");
  Mantid::Kernel::DblMatrix wMatrix(wMatrixAsArray);

  // Get the B Matrix from the oriented lattice
  const auto &orientedLattice = sample.getOrientedLattice();
  Mantid::Kernel::DblMatrix bMatrix = orientedLattice.getB();
  bMatrix *= wMatrix;

  // Get G* Matrix
  Mantid::Kernel::DblMatrix gStarMatrix = bMatrix.Tprime() * bMatrix;

  // Get recalculated BMatrix from Unit Cell
  Mantid::Geometry::UnitCell unitCell(orientedLattice);
  unitCell.recalculateFromGstar(gStarMatrix);
  skewMatrix = unitCell.getB();

  // Provide column normalisation of the skewMatrix
  normalizeColumns(skewMatrix);

  // Setup basis normalisation array
  std::vector<double> basisNormalization = {orientedLattice.astar(),
                                            orientedLattice.bstar(),
                                            orientedLattice.cstar()};

  // Expand matrix to 4 dimensions if necessary
  if (4 == workspace->getNumDims()) {
    basisNormalization.push_back(1.0);
    Mantid::Kernel::DblMatrix temp(4, 4, true);
    for (std::size_t i = 0; i < 3; i++) {
      for (std::size_t j = 0; j < 3; j++) {
        temp[i][j] = skewMatrix[i][j];
      }
    }
    skewMatrix = temp;
  }

  // The affine matrix has a underlying type of coord_t(float) but
  // we need a double

  auto reducedDimension = affineMatrix.Ssize() - 1;
  Mantid::Kernel::DblMatrix affMat(reducedDimension, reducedDimension);
  for (std::size_t i = 0; i < reducedDimension; i++) {
    for (std::size_t j = 0; j < reducedDimension; j++) {
      affMat[i][j] = affineMatrix[i][j];
    }
  }

  // Perform similarity transform to get coordinate orientation correct
  skewMatrix = affMat.Tprime() * (skewMatrix * affMat);

  if (4 == workspace->getNumDims()) {
    stripMatrix(skewMatrix);
  }
  skewMatrix
      .Invert(); // Current fix so skewed image displays in correct orientation
}

template <typename T> bool doRequiresSkewMatrix(T workspace) {
  auto requiresSkew(true);
  try {
    const auto &sample = workspace->getExperimentInfo(0)->sample();
    const auto &run = workspace->getExperimentInfo(0)->run();
    auto specialCoordnateSystem = workspace->getSpecialCoordinateSystem();
    checkForSampleAndRunEntries(sample, run, specialCoordnateSystem);
  } catch (std::invalid_argument &) {
    requiresSkew = false;
  }

  return requiresSkew;
}

template <typename T, size_t N>
std::array<T, N> getTransformedArray(T skewMatrix[N * N], size_t dimension) {
  std::array<T, N> vec = {0., 0., 0.};
  for (size_t index = 0; index < N; ++index) {
    vec[index] = skewMatrix[dimension + index * N];
  }
  return vec;
}

template <typename T, size_t N> void normalizeVector(std::array<T, N> &vector) {
  auto sumOfSquares =
      [](double sum, double element) { return sum + element * element; };
  auto norm = std::accumulate(vector.begin(), vector.end(), 0.f, sumOfSquares);
  norm = std::sqrt(norm);
  for (auto &element : vector) {
    element /= norm;
  }
}
template <typename T>
std::array<T, 3> getNormalVector(std::array<T, 3> &dimXTransformed,
                                 std::array<T, 3> &dimYTransformed) {
  std::array<T, 3> normalVector;
  for (size_t index = 0; index < 3; ++index) {
    normalVector[index] =
        dimXTransformed[(index + 1) % 3] * dimYTransformed[(index + 2) % 3] -
        dimXTransformed[(index + 2) % 3] * dimYTransformed[(index + 1) % 3];
  }

  // Make sure that the output is truely normalized
  normalizeVector<T, 3>(normalVector);
  return normalVector;
}

/**
  * Calculate the angle for a given dimension. Note that this function expects
 * all vectors to be normalized.
  */
template <typename T, size_t N>
T getAngleInRadian(std::array<T, N> vector1, std::array<T, N> vector2,
                   std::array<T, N> normalVector) {
  normalizeVector<T, N>(vector1);
  normalizeVector<T, N>(vector2);

  // Get the value of the angle from the dot product: v1*v2 = cos (a)*|v1|*|v2|
  auto dotProduct =
      std::inner_product(vector1.begin(), vector1.end(), vector2.begin(), 0.f);

  // Handle case where the dotProduct is 1 or -1
  T angle = 0.;
  if (dotProduct == 1.) {
    angle = 0.;

  } else if (dotProduct == -1.) {
    angle = static_cast<float>(M_PI);

  } else {
    angle = std::acos(dotProduct);
  }
  // Get the direction of the angle
  auto normalForDirection = getNormalVector<Mantid::coord_t>(vector1, vector2);
  auto direction =
      std::inner_product(normalForDirection.begin(), normalForDirection.end(),
                         normalVector.begin(), 0.f);

  if (direction < 0) {
    angle *= -1.f;
  }
  return angle;
}

}

namespace MantidQt {
namespace API {

void provideSkewMatrix(Mantid::Kernel::DblMatrix &skewMatrix,
                       Mantid::API::IMDWorkspace_const_sptr workspace) {
  if (Mantid::API::IMDEventWorkspace_const_sptr eventWorkspace =
          boost::dynamic_pointer_cast<const Mantid::API::IMDEventWorkspace>(
              workspace)) {
    doProvideSkewMatrix(skewMatrix, eventWorkspace);
  } else if (Mantid::API::IMDHistoWorkspace_const_sptr histoWorkspace =
                 boost::dynamic_pointer_cast<
                     const Mantid::API::IMDHistoWorkspace>(workspace)) {
    doProvideSkewMatrix(skewMatrix, histoWorkspace);
  } else {
    throw std::invalid_argument("NonOrthogonal: The provided workspace "
                                "must either be an IMDEvent or IMDHisto "
                                "workspace.");
  }
}

bool requiresSkewMatrix(Mantid::API::IMDWorkspace_const_sptr workspace) {

  auto requiresSkewMatrix(false);
  if (Mantid::API::IMDEventWorkspace_const_sptr eventWorkspace =
          boost::dynamic_pointer_cast<const Mantid::API::IMDEventWorkspace>(
              workspace)) {
    requiresSkewMatrix = doRequiresSkewMatrix(eventWorkspace);
  } else if (Mantid::API::IMDHistoWorkspace_const_sptr histoWorkspace =
                 boost::dynamic_pointer_cast<
                     const Mantid::API::IMDHistoWorkspace>(workspace)) {
    requiresSkewMatrix = doRequiresSkewMatrix(histoWorkspace);
  }
  return requiresSkewMatrix;
}
bool isHKLDimensions(Mantid::API::IMDWorkspace_const_sptr workspace,
                     size_t dimX, size_t dimY) {
  auto dimensionHKL = true;
  size_t dimensionIndices[2] = {dimX, dimY};
  for (const auto &dimensionIndex : dimensionIndices) {
    auto dimension = workspace->getDimension(dimensionIndex);
    const auto &frame = dimension->getMDFrame();
    if (frame.name() != Mantid::Geometry::HKL::HKLName) {
      dimensionHKL = false;
    }
  }
  return dimensionHKL;
}

void transformFromDoubleToCoordT(Mantid::Kernel::DblMatrix &skewMatrix,
                                 Mantid::coord_t skewMatrixCoord[9]) {
  std::size_t index = 0;
  for (std::size_t i = 0; i < skewMatrix.numRows(); ++i) {
    for (std::size_t j = 0; j < skewMatrix.numCols(); ++j) {
      skewMatrixCoord[index] = static_cast<Mantid::coord_t>(skewMatrix[i][j]);
      ++index;
    }
  }
}
/*
template <typename T>
void transformLookpointToWorkspaceCoordGeneric(T &lookPoint,
        Mantid::coord_t skewMatrix[9], size_t &dimX, size_t &dimY) {
        auto v1 = lookPoint[0];
        auto v2 = lookPoint[1];
        auto v3 = lookPoint[2];
        lookPoint[dimX] = v1 * skewMatrix[0 + 3 * dimX] +
                v2 * skewMatrix[1 + 3 * dimX] +
                v3 * skewMatrix[2 + 3 * dimX];
        lookPoint[dimY] = v1 * skewMatrix[0 + 3 * dimY] +
                v2 * skewMatrix[1 + 3 * dimY] +
                v3 * skewMatrix[2 + 3 * dimY];
}*/
void transformVMDToWorkspaceCoord(Mantid::Kernel::VMD &lookPoint,
                                  Mantid::coord_t skewMatrix[9], size_t &dimX,
                                  size_t &dimY) {
  transformLookpointToWorkspaceCoordGeneric(lookPoint, skewMatrix, dimX, dimY);
}

void transformLookpointToWorkspaceCoord(Mantid::coord_t *lookPoint,
                                        const Mantid::coord_t skewMatrix[9],
                                        const size_t &dimX,
                                        const size_t &dimY) {

  // transformLookpointToWorkspaceCoordGeneric(&lookPoint, skewMatrix, dimX,
  // dimY);
  auto v1 = lookPoint[0];
  auto v2 = lookPoint[1];
  auto v3 = lookPoint[2];
  lookPoint[dimX] = v1 * skewMatrix[0 + 3 * dimX] +
                    v2 * skewMatrix[1 + 3 * dimX] +
                    v3 * skewMatrix[2 + 3 * dimX];
  lookPoint[dimY] = v1 * skewMatrix[0 + 3 * dimY] +
                    v2 * skewMatrix[1 + 3 * dimY] +
                    v3 * skewMatrix[2 + 3 * dimY];
}

std::pair<float, float> getAnglesInRadian(Mantid::coord_t skewMatrixCoord[9],
                                          size_t dimX, size_t dimY) {
  std::array<Mantid::coord_t, 3> dimXOriginal = {0., 0., 0.};
  std::array<Mantid::coord_t, 3> dimYOriginal = {0., 0., 0.};
  dimXOriginal[dimX] = 1.0;
  dimYOriginal[dimY] = 1.0;

  // Get the tranformed values
  auto dimXTransformed =
      getTransformedArray<Mantid::coord_t, 3>(skewMatrixCoord, dimX);
  auto dimYTransformed =
      getTransformedArray<Mantid::coord_t, 3>(skewMatrixCoord, dimY);

  // Calculate the normal vector of the the transformed environment
  auto normalVector =
      getNormalVector<Mantid::coord_t>(dimXTransformed, dimYTransformed);

  // Get the angle for dimX and dimY
  auto angleDimX =
      getAngleInRadian(dimXOriginal, dimXTransformed, normalVector);
  auto angleDimY =
      getAngleInRadian(dimYOriginal, dimYTransformed, normalVector);
  return std::make_pair(angleDimX, angleDimY);
}
}
}
