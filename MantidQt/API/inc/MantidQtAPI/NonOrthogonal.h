#ifndef MANTID_MANTIDQT_API_NON_ORTHOGONAL_H_
#define MANTID_MANTIDQT_API_NON_ORTHOGONAL_H_

#include "MantidQtAPI/DllOption.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/Matrix.h"

namespace MantidQt {
namespace API {
void EXPORT_OPT_MANTIDQT_API
provideSkewMatrix(Mantid::Kernel::DblMatrix &skewMatrix,
                  Mantid::API::IMDWorkspace_const_sptr workspace);

bool EXPORT_OPT_MANTIDQT_API
requiresSkewMatrix(Mantid::API::IMDWorkspace_const_sptr workspace);

bool EXPORT_OPT_MANTIDQT_API
isHKLDimensions(Mantid::API::IMDWorkspace_const_sptr workspace, size_t dimX,
                size_t dimY);

void EXPORT_OPT_MANTIDQT_API
transformFromDoubleToCoordT(Mantid::Kernel::DblMatrix &skewMatrix,
                            Mantid::coord_t skewMatrixCoord[9]);

void EXPORT_OPT_MANTIDQT_API
transformVMDToWorkspaceCoord(Mantid::Kernel::VMD &lookPoint,
	Mantid::coord_t skewMatrix[9], size_t &dimX, size_t &dimY);

void EXPORT_OPT_MANTIDQT_API
transformLookpointToWorkspaceCoord(Mantid::coord_t *lookPoint,
	const Mantid::coord_t skewMatrix[9], const size_t &dimX, const size_t &dimY);

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
}
}
}

#endif
