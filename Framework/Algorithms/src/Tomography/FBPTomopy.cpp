#include "MantidAlgorithms/Tomography/FBPTomopy.h"
#include "MantidAlgorithms/Tomography/tomopy/fbp.h"

namespace Mantid {
namespace Algorithms {
namespace Tomography {

void FBPTomopy(const float *data, int dy, int dt, int dx, const float *center,
               const float *theta, float *recon, int ngridx, int ngridy) {
  fbp(data, dy, dt, dx, center, theta, recon, ngridx, ngridy, nullptr, nullptr);
}

} // namespace Tomography
} // namespace Algorithms
} // namespace Mantid
