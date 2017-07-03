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

#include <boost/pointer_cast.hpp>
#include <algorithm>
#include <array>
#include <cmath>

/**
*
* There are several things which should be clarified.
*
* 1. We are dealing with a (potentially non-orthogonal system) defined by
* the basisi vectors a*, b* and c* with the coordinates h, k, l. On occasion
* H, K, and L are used to describe the basisi vectors.
*
* 2. What we call a skewMatrix is a modified BW (and sometimes a modified
*(BW)^-1) matrix.
* The BW matrix transforms from the non-orhtogonal presentation to the
*orthogonal representation.
* The (BW)^-1 transforms form the orhtogonal presentation to the non-orthogonal
*representation.
* The orthogonal representation is a orthogonal coordinate system with
*coordinates (x, y, z),
* where the basis vector eX assoicated with x is aligned with the H. The basis
*vector eY
* associated with y is in the H-K plane and perpendicular to x. The basis vector
*eZ associated
* with z is orthgonal to x and y.
*
* This is important. H is always parallel to eX, K is always in the x-y plane
*and L can be pretty
* much anything.
*
* 3. The screen coordinate system consists of Xs and Ys.
*
*
*/

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

  // Apply column normalisation to skew matrix
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
void doProvideSkewMatrix(Mantid::Kernel::DblMatrix &skewMatrix,
                         const T &workspace) {
  // The input workspace needs have
  // 1. an HKL frame
  // 2. an oriented lattice
  // else we cannot create the skew matrix
  const auto &sample = workspace.getExperimentInfo(0)->sample();
  const auto &run = workspace.getExperimentInfo(0)->run();
  auto specialCoordnateSystem = workspace.getSpecialCoordinateSystem();
  checkForSampleAndRunEntries(sample, run, specialCoordnateSystem);

  // Create Affine Matrix
  Mantid::Kernel::Matrix<Mantid::coord_t> affineMatrix;
  try {
    auto const *transform = workspace.getTransformToOriginal();
    affineMatrix = transform->makeAffineMatrix();
  } catch (std::runtime_error &) {
    // Create identity matrix of dimension+1
    std::size_t nDims = workspace.getNumDims() + 1;
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
  if (4 == workspace.getNumDims()) {
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

  if (4 == workspace.getNumDims()) {
    stripMatrix(skewMatrix);
  }
  // Current fix so skewed image displays in correct orientation
  skewMatrix.Invert();
}

template <typename T> bool doRequiresSkewMatrix(const T &workspace) {
  auto requiresSkew(true);
  try {
    const auto &sample = workspace.getExperimentInfo(0)->sample();
    const auto &run = workspace.getExperimentInfo(0)->run();
    auto specialCoordnateSystem = workspace.getSpecialCoordinateSystem();
    checkForSampleAndRunEntries(sample, run, specialCoordnateSystem);
  } catch (std::invalid_argument &) {
    requiresSkew = false;
  }

  return requiresSkew;
}

template <size_t N>
std::array<Mantid::coord_t, N>
getTransformedArray(const std::array<Mantid::coord_t, N * N> &skewMatrix,
                    size_t dimension) {
  std::array<Mantid::coord_t, N> vec = {{0., 0., 0.}};
  for (size_t index = 0; index < N; ++index) {
    vec[index] = skewMatrix[dimension + index * N];
  }
  return vec;
}

template <typename T, size_t N> void normalizeVector(std::array<T, N> &vector) {
  auto sumOfSquares = [](T sum, T element) { return sum + element * element; };
  auto norm = std::accumulate(vector.begin(), vector.end(), 0.f, sumOfSquares);
  norm = std::sqrt(norm);
  for (auto &element : vector) {
    element /= norm;
  }
}
/**
* Gets the normal vector for two specified vectors
*
*/
std::array<Mantid::coord_t, 3>
    getNormalVector(std::array<Mantid::coord_t, 3> vector1,
                    std::array<Mantid::coord_t, 3> vector2) {
  std::array<Mantid::coord_t, 3> normalVector;
  for (size_t index = 0; index < 3; ++index) {
    normalVector[index] = vector1[(index + 1) % 3] * vector2[(index + 2) % 3] -
                          vector1[(index + 2) % 3] * vector2[(index + 1) % 3];
  }

  // Make sure that the output is truely normalized
  normalizeVector<Mantid::coord_t, 3>(normalVector);
  return normalVector;
}

/**
* The normal vector will depend on the chosen dimensions and the order of these
*dimensions:
* It is essentially the cross product of vect(dimX) x vect(dimY), e.g.
* x-y -> z
* y-x -> -z ...
*
*/
std::array<Mantid::coord_t, 3> getNormalVector(size_t dimX, size_t dimY) {
  std::array<Mantid::coord_t, 3> vector1 = {{0., 0., 0.}};
  std::array<Mantid::coord_t, 3> vector2 = {{0., 0., 0.}};
  vector1[dimX] = 1.0;
  vector2[dimY] = 1.0;

  std::array<Mantid::coord_t, 3> normalVector;
  for (size_t index = 0; index < 3; ++index) {
    normalVector[index] = vector1[(index + 1) % 3] * vector2[(index + 2) % 3] -
                          vector1[(index + 2) % 3] * vector2[(index + 1) % 3];
  }

  // Make sure that the output is normalized
  normalizeVector<Mantid::coord_t, 3>(normalVector);
  return normalVector;
}
/**
* Calculate the angle for a given dimension. Note that this function expects all
* vectors to be normalized.
*/
template <size_t N>
double getAngleInRadian(std::array<Mantid::coord_t, N> orthogonalVector,
                        std::array<Mantid::coord_t, N> nonOrthogonalVector,
                        const std::array<Mantid::coord_t, N> &normalVector,
                        size_t currentDimension, size_t otherDimension) {
  // We want to get the angle between an orthogonal basis vector eX, eY, eZ and
  // the corresponding
  // nonorthogonal basis vector H, K, L
  // There are several special cases to consider.
  // 1. When the currentDimension or otherDimension is x, then the angle is 0
  // since x and H are aligned
  // 2. When currentDimension is y and otherDimension is z,
  //    then the angle betwee K and eY  is set to 0. This is a slight
  //    oddity since  y-z and K are not in a plane. Mathematically, there is of
  //    course a potentially non-zero
  //    angle between K and eY, but this is not relevant for our 2D display.
  // 3. When dimX/Y is z, then L needs to be projected onto either the x-z or
  // the y-z plane (denpending
  //    on the current selection). The angle is calculatd between the projection
  //    and the eZ axis.

  double angle(0.);
  if (currentDimension == 0) {
    // Handle case 1.
    return 0.;
  } else if (currentDimension == 1 && otherDimension == 2) {
    // Handle case 2.
    return 0.;
  } else if (currentDimension == 2) {
    // Handle case 3.
    normalizeVector<Mantid::coord_t, N>(orthogonalVector);
    normalizeVector<Mantid::coord_t, N>(nonOrthogonalVector);

    // projecting onto third dimension by setting dimension coming out of screen
    // to zero
    std::array<Mantid::coord_t, 3> temporaryNonOrthogonal{{0.f, 0.f, 0.f}};
    temporaryNonOrthogonal[currentDimension] =
        nonOrthogonalVector[currentDimension];
    temporaryNonOrthogonal[otherDimension] =
        nonOrthogonalVector[otherDimension];
    nonOrthogonalVector = temporaryNonOrthogonal;
  }

  normalizeVector<Mantid::coord_t, N>(orthogonalVector);
  normalizeVector<Mantid::coord_t, N>(nonOrthogonalVector);
  // Get the value of the angle from the dot product: v1*v2 = cos (a)*|v1|*|v2|
  auto dotProduct =
      std::inner_product(orthogonalVector.begin(), orthogonalVector.end(),
                         nonOrthogonalVector.begin(), 0.f);

  // Handle case where the dotProduct is 1 or -
  if (dotProduct == 1.) {
    angle = 0.;

  } else if (dotProduct == -1.) {
    angle = static_cast<double>(M_PI);

  } else {
    angle = std::acos(dotProduct);
  }
  // Get the direction of the angle
  auto normalForDirection =
      getNormalVector(orthogonalVector, nonOrthogonalVector);
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

size_t
getMissingHKLDimensionIndex(Mantid::API::IMDWorkspace_const_sptr workspace,
                            size_t dimX, size_t dimY) {
  for (size_t i = 0; i < workspace->getNumDims(); ++i) {
    auto dimension = workspace->getDimension(i);
    const auto &frame = dimension->getMDFrame();
    if ((frame.name() == Mantid::Geometry::HKL::HKLName) && (i != dimX) &&
        (i != dimY)) {
      return i;
    }
  }
  return static_cast<size_t>(NULL);
}

void provideSkewMatrix(Mantid::Kernel::DblMatrix &skewMatrix,
                       const Mantid::API::IMDWorkspace &workspace) {
  if (auto mdew =
          dynamic_cast<const Mantid::API::IMDEventWorkspace *>(&workspace)) {
    doProvideSkewMatrix(skewMatrix, *mdew);
  } else if (auto mdhw = dynamic_cast<const Mantid::API::IMDHistoWorkspace *>(
                 &workspace)) {
    doProvideSkewMatrix(skewMatrix, *mdhw);
  } else {
    throw std::invalid_argument("NonOrthogonal: The provided workspace "
                                "must either be an IMDEvent or IMDHisto "
                                "workspace.");
  }
}

bool requiresSkewMatrix(const Mantid::API::IMDWorkspace &workspace) {
  if (auto mdew =
          dynamic_cast<const Mantid::API::IMDEventWorkspace *>(&workspace)) {
    return doRequiresSkewMatrix(*mdew);
  } else if (auto mdhw = dynamic_cast<const Mantid::API::IMDHistoWorkspace *>(
                 &workspace)) {
    return doRequiresSkewMatrix(*mdhw);
  } else {
    return false;
  }
}

bool isHKLDimensions(const Mantid::API::IMDWorkspace &workspace, size_t dimX,
                     size_t dimY) {
  auto hklname = [&workspace](size_t index) {
    return workspace.getDimension(index)->getMDFrame().name() ==
           Mantid::Geometry::HKL::HKLName;
  };
  return hklname(dimX) && hklname(dimY);
}

void transformFromDoubleToCoordT(
    const Mantid::Kernel::DblMatrix &skewMatrix,
    std::array<Mantid::coord_t, 9> &skewMatrixCoord) {
  std::size_t index = 0;
  for (std::size_t i = 0; i < skewMatrix.numRows(); ++i) {
    for (std::size_t j = 0; j < skewMatrix.numCols(); ++j) {
      skewMatrixCoord[index] = static_cast<Mantid::coord_t>(skewMatrix[i][j]);
      ++index;
    }
  }
}
/**
Explanation of index mapping
lookPoint[0] is H
lookPoint[1] is K
lookPoint[2] is L
eg
Have matrix (from xyz -> hkl) and X, K, Z
H = M11 . X + M12Y + M13Z
K = M24X + M22Y + M23Z
L = M31X + M32Y + M33Z

*/
void transformLookpointToWorkspaceCoord(
    Mantid::coord_t *lookPoint,
    const std::array<Mantid::coord_t, 9> &skewMatrix, const size_t &dimX,
    const size_t &dimY, const size_t &dimSlice) {

  auto sliceDimResult =
      (lookPoint[dimSlice] - skewMatrix[3 * dimSlice + dimX] * lookPoint[dimX] -
       skewMatrix[3 * dimSlice + dimY] * lookPoint[dimY]) /
      skewMatrix[3 * dimSlice + dimSlice];

  auto OrigDimSliceValue = lookPoint[dimSlice];
  lookPoint[dimSlice] = sliceDimResult;

  auto v1 = lookPoint[0];
  auto v2 = lookPoint[1];
  auto v3 = lookPoint[2];

  lookPoint[dimX] = v1 * skewMatrix[0 + 3 * dimX] +
                    v2 * skewMatrix[1 + 3 * dimX] +
                    v3 * skewMatrix[2 + 3 * dimX];
  lookPoint[dimY] = v1 * skewMatrix[0 + 3 * dimY] +
                    v2 * skewMatrix[1 + 3 * dimY] +
                    v3 * skewMatrix[2 + 3 * dimY];

  lookPoint[dimSlice] = OrigDimSliceValue;
}

/**
* We get the angles that are used for plotting of the grid lines. There are
*several scenarios:
* x-y (when H and K are selected)
* y-x (when K and H are selected)
* x-z (when H and L are selected)
* z-x (when L and H are selected)
* y-z (when K and L are selected)
* z-y (when L and K are selected)
*
* Some things to consider the BW transformation provides a system where x is
*aligned with a*,
* where y is in the same plane as
*
*
* @param skewMatrixCoord The tranformation matrix from the non-orthogonal system
*to the orthogonal system
* @param dimX the selected orthogonal dimension for the x axis of the screen
* @param dimY the selected orthogonal dimension for the y axis of the screen
* @return an angle for the x grid lines and an angle for the y grid lines. Both
*are measured from the x axis.
*/
std::pair<double, double>
getGridLineAnglesInRadian(const std::array<Mantid::coord_t, 9> &skewMatrixCoord,
                          size_t dimX, size_t dimY) {
  // Get the two vectors for the selected dimensions in the orthogonal axis
  // representation.

  std::array<Mantid::coord_t, 3> dimXOriginal = {{0., 0., 0.}};
  std::array<Mantid::coord_t, 3> dimYOriginal = {{0., 0., 0.}};
  dimXOriginal[dimX] = 1.0;
  dimYOriginal[dimY] = 1.0;
  auto dimXTransformed = getTransformedArray<3>(skewMatrixCoord, dimX);
  auto dimYTransformed = getTransformedArray<3>(skewMatrixCoord, dimY);

  // Get the normal vector for the selected dimensions
  auto normalVector = getNormalVector(dimX, dimY);

  // Get the angle for dimX and dimY
  auto angleDimX =
      getAngleInRadian(dimXOriginal, dimXTransformed, normalVector, dimX, dimY);
  auto angleDimY =
      getAngleInRadian(dimYOriginal, dimYTransformed, normalVector, dimY, dimX);
  return std::make_pair(angleDimX, angleDimY);
}
}
}
