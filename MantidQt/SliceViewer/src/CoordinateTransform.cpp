#include "MantidQtSliceViewer/CoordinateTransform.h"
#include "MantidQtAPI/NonOrthogonal.h"
#include "MantidKernel/make_unique.h"
#include "MantidGeometry/MDGeometry/HKL.h"

namespace MantidQt {
namespace SliceViewer {
NullTransform::~NullTransform() {}

void NullTransform::transform(Mantid::Kernel::VMD &coords, size_t dimX,
                              size_t dimY) {}
void NullTransform::checkDimensionsForHKL(Mantid::API::IMDWorkspace_sptr ws,
                                          size_t dimX, size_t dimY){};

NonOrthogonalTransform::~NonOrthogonalTransform() {}

NonOrthogonalTransform::NonOrthogonalTransform(
    Mantid::API::IMDWorkspace_sptr ws, size_t dimX, size_t dimY)
    : m_dimensionsHKL(true) {
  // Set the skewMatrix for the non-orthogonal data
  auto numberOfDimensions = ws->getNumDims();
  Mantid::Kernel::DblMatrix skewMatrix(numberOfDimensions, numberOfDimensions,
                                       true);
  API::provideSkewMatrix(skewMatrix, ws);

  // Transform from double to coord_t
  std::size_t index = 0;
  for (std::size_t i = 0; i < skewMatrix.numRows(); ++i) {
    for (std::size_t j = 0; j < skewMatrix.numCols(); ++j) {
      m_skewMatrix[index] = static_cast<Mantid::coord_t>(skewMatrix[i][j]);
      ++index;
    }
  }
  checkDimensionsForHKL(ws, dimX, dimY);
}
void NonOrthogonalTransform::checkDimensionsForHKL(
    Mantid::API::IMDWorkspace_sptr ws, size_t dimX, size_t dimY) {
  auto dimensionHKL = true;
  size_t dimensionIndices[2] = {dimX, dimY};
  for (auto dimensionIndex : dimensionIndices) {
    auto dimension = ws->getDimension(dimensionIndex);
    const auto &frame = dimension->getMDFrame();
    if (frame.name() != Mantid::Geometry::HKL::HKLName) {
      dimensionHKL = false;
    }
  }
  m_dimensionsHKL = dimensionHKL;
}
void NonOrthogonalTransform::transform(Mantid::Kernel::VMD &coords, size_t dimX,
                                       size_t dimY) {
  if (m_dimensionsHKL) {
    auto v1 = coords[0];
    auto v2 = coords[1];
    auto v3 = coords[2];
    coords[dimX] = v1 * m_skewMatrix[0 + 3 * dimX] +
                   v2 * m_skewMatrix[1 + 3 * dimX] +
                   v3 * m_skewMatrix[2 + 3 * dimX];
    coords[dimY] = v1 * m_skewMatrix[0 + 3 * dimY] +
                   v2 * m_skewMatrix[1 + 3 * dimY] +
                   v3 * m_skewMatrix[2 + 3 * dimY];
  }
}

std::unique_ptr<CoordinateTransform>
createCoordinateTransform(Mantid::API::IMDWorkspace_sptr ws, size_t dimX,
                          size_t dimY) {
  std::unique_ptr<CoordinateTransform> coordinateTransform;
  if (API::requiresSkewMatrix(ws)) {
    coordinateTransform =
        Mantid::Kernel::make_unique<NonOrthogonalTransform>(ws, dimX, dimY);
  } else {
    coordinateTransform = Mantid::Kernel::make_unique<NullTransform>();
  }
  return coordinateTransform;
}
}
}
