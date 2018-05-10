#ifndef MANTID_MANTIDQT_API_NON_ORTHOGONAL_H_
#define MANTID_MANTIDQT_API_NON_ORTHOGONAL_H_

#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/Matrix.h"
#include "MantidQtWidgets/Common/DllOption.h"
#include <array>

namespace MantidQt {
namespace API {

enum class DimensionSelection { H, K, L };

void EXPORT_OPT_MANTIDQT_COMMON
provideSkewMatrix(Mantid::Kernel::DblMatrix &skewMatrix,
                  const Mantid::API::IMDWorkspace &workspace);

bool EXPORT_OPT_MANTIDQT_COMMON
requiresSkewMatrix(const Mantid::API::IMDWorkspace &workspace);

bool EXPORT_OPT_MANTIDQT_COMMON isHKLDimensions(
    const Mantid::API::IMDWorkspace &workspace, size_t dimX, size_t dimY);

size_t EXPORT_OPT_MANTIDQT_COMMON getMissingHKLDimensionIndex(
    Mantid::API::IMDWorkspace_const_sptr workspace, size_t dimX, size_t dimY);

void EXPORT_OPT_MANTIDQT_COMMON
transformFromDoubleToCoordT(const Mantid::Kernel::DblMatrix &skewMatrix,
                            std::array<Mantid::coord_t, 9> &skewMatrixCoord);

template <typename T>
void transformLookpointToWorkspaceCoord(
    T &lookPoint, const std::array<Mantid::coord_t, 9> &skewMatrix,
    const size_t &dimX, const size_t &dimY, const size_t &dimSlice) {
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

std::pair<double, double> EXPORT_OPT_MANTIDQT_COMMON
getGridLineAnglesInRadian(const std::array<Mantid::coord_t, 9> &skewMatrixCoord,
                          size_t dimX, size_t dimY);
} // namespace API
} // namespace MantidQt

#endif
