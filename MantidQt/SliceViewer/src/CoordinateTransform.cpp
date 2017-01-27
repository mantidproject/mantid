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
void NullTransform::checkDimensionsForHKL(Mantid::API::IMDWorkspace_sptr ws,
                                          size_t dimX, size_t dimY) {
  (void)ws;
  (void)dimX;
  (void)dimY;
}

NonOrthogonalTransform::~NonOrthogonalTransform() {}

NonOrthogonalTransform::NonOrthogonalTransform(
    Mantid::API::IMDWorkspace_sptr ws, size_t dimX, size_t dimY)
    : m_dimensionsHKL(true) {
  // Set the skewMatrix for the non-orthogonal data
  auto numberOfDimensions = ws->getNumDims();
  Mantid::Kernel::DblMatrix skewMatrix(numberOfDimensions, numberOfDimensions,
                                       true);
  API::provideSkewMatrix(skewMatrix, ws);
  API::transformFromDoubleToCoordT(skewMatrix, m_skewMatrix);
  checkDimensionsForHKL(ws, dimX, dimY);
}
void NonOrthogonalTransform::checkDimensionsForHKL(
    Mantid::API::IMDWorkspace_sptr ws, size_t dimX, size_t dimY) {
  bool dimensionHKL = API::isHKLDimensions(ws, dimX, dimY);
  m_dimensionsHKL = dimensionHKL;
}
void NonOrthogonalTransform::transform(Mantid::Kernel::VMD &coords, size_t dimX,
                                       size_t dimY, size_t missingHKLDim) {
  if (m_dimensionsHKL) {
    API::transformLookpointToWorkspaceCoordGeneric(coords, m_skewMatrix, dimX,
                                                   dimY, missingHKLDim);
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
