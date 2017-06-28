#include "MantidQtSliceViewer/CoordinateTransform.h"
#include "MantidQtAPI/NonOrthogonal.h"
#include "MantidKernel/make_unique.h"
#include "MantidGeometry/MDGeometry/HKL.h"

namespace MantidQt {
namespace SliceViewer {

void NullTransform::transform(Mantid::Kernel::VMD &coords, size_t dimX,
                              size_t dimY, size_t missingHKLDim) {
  (void)coords;
  (void)dimX;
  (void)dimY;
  (void)missingHKLDim;
}
void NullTransform::checkDimensionsForHKL(
    const Mantid::API::IMDWorkspace_const_sptr &ws, size_t dimX, size_t dimY) {
  (void)ws;
  (void)dimX;
  (void)dimY;
}

NonOrthogonalTransform::~NonOrthogonalTransform() {}

NonOrthogonalTransform::NonOrthogonalTransform(
    const Mantid::API::IMDWorkspace_const_sptr &workspace, size_t dimX,
    size_t dimY)
    : m_dimensionsHKL(true), m_skewMatrix({0}) {
  // Set the skewMatrix for the non-orthogonal data
  auto numberOfDimensions = workspace->getNumDims();
  Mantid::Kernel::DblMatrix skewMatrix(numberOfDimensions, numberOfDimensions,
                                       true);
  ///@cond
  API::provideSkewMatrix(skewMatrix, workspace);
  API::transformFromDoubleToCoordT(skewMatrix, m_skewMatrix);
  ///@endcond
  checkDimensionsForHKL(workspace, dimX, dimY);
}
void NonOrthogonalTransform::checkDimensionsForHKL(
    const Mantid::API::IMDWorkspace_const_sptr &ws, size_t dimX, size_t dimY) {
  bool dimensionHKL = API::isHKLDimensions(ws, dimX, dimY);
  m_dimensionsHKL = dimensionHKL;
}
void NonOrthogonalTransform::transform(Mantid::Kernel::VMD &coords, size_t dimX,
                                       size_t dimY, size_t missingHKLDim) {
  if (m_dimensionsHKL) {
    API::transformLookpointToWorkspaceCoord(coords, m_skewMatrix, dimX, dimY,
                                            missingHKLDim);
  }
}

std::unique_ptr<CoordinateTransform>
createCoordinateTransform(const Mantid::API::IMDWorkspace_sptr &ws, size_t dimX,
                          size_t dimY) {
  std::unique_ptr<CoordinateTransform> coordinateTransform;
  if (MantidQt::API::requiresSkewMatrix(ws)) {
    coordinateTransform =
        Mantid::Kernel::make_unique<NonOrthogonalTransform>(ws, dimX, dimY);
  } else {
    coordinateTransform = Mantid::Kernel::make_unique<NullTransform>();
  }
  return coordinateTransform;
}
}
}
