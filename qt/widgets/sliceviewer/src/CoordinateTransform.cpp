// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/SliceViewer/CoordinateTransform.h"
#include "MantidGeometry/MDGeometry/HKL.h"

#include "MantidQtWidgets/Common/NonOrthogonal.h"

namespace MantidQt {
namespace SliceViewer {

void NullTransform::transform(Mantid::Kernel::VMD &coords, size_t dimX,
                              size_t dimY, size_t missingHKLDim) {
  (void)coords;
  (void)dimX;
  (void)dimY;
  (void)missingHKLDim;
}
void NullTransform::checkDimensionsForHKL(const Mantid::API::IMDWorkspace &ws,
                                          size_t dimX, size_t dimY) {
  (void)ws;
  (void)dimX;
  (void)dimY;
}

NonOrthogonalTransform::~NonOrthogonalTransform() {}

NonOrthogonalTransform::NonOrthogonalTransform(
    const Mantid::API::IMDWorkspace &workspace, size_t dimX, size_t dimY)
    : m_dimensionsHKL(true), m_skewMatrix() {
  // Set the skewMatrix for the non-orthogonal data
  auto numberOfDimensions = workspace.getNumDims();
  Mantid::Kernel::DblMatrix skewMatrix(numberOfDimensions, numberOfDimensions,
                                       true);
  ///@cond
  API::provideSkewMatrix(skewMatrix, workspace);
  API::transformFromDoubleToCoordT(skewMatrix, m_skewMatrix);
  ///@endcond
  checkDimensionsForHKL(workspace, dimX, dimY);
}
void NonOrthogonalTransform::checkDimensionsForHKL(
    const Mantid::API::IMDWorkspace &ws, size_t dimX, size_t dimY) {
  m_dimensionsHKL = API::isHKLDimensions(ws, dimX, dimY);
}
void NonOrthogonalTransform::transform(Mantid::Kernel::VMD &coords, size_t dimX,
                                       size_t dimY, size_t missingHKLDim) {
  if (m_dimensionsHKL) {
    API::transformLookpointToWorkspaceCoord(coords, m_skewMatrix, dimX, dimY,
                                            missingHKLDim);
  }
}

std::unique_ptr<CoordinateTransform>
createCoordinateTransform(const Mantid::API::IMDWorkspace &ws, size_t dimX,
                          size_t dimY) {
  if (MantidQt::API::requiresSkewMatrix(ws)) {
    return std::make_unique<NonOrthogonalTransform>(ws, dimX, dimY);
  } else {
    return std::make_unique<NullTransform>();
  }
}
} // namespace SliceViewer
} // namespace MantidQt
