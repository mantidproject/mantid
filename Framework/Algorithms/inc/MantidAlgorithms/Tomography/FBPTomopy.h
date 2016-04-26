#ifndef MANTID_ALGORITHMS_TOMOGRAPHY_FBPTOMOPY_H_
#define MANTID_ALGORITHMS_TOMOGRAPHY_FBPTOMOPY_H_

#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {
namespace Tomography {

/// FBP - Filtered Back-Projection routine as implemented in tomopy
void MANTID_ALGORITHMS_DLL
FBPTomopy(const float *data, int dy, int dt, int dx, const float *center,
          const float *theta, float *recon, int ngridx, int ngridy);

} // namespace Tomography
} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_TOMOGRAPHY_FBPTOMOPY_H_ */
