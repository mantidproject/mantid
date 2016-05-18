#ifndef MANTID_ALGORITHMS_TOMOGRAPHY_FBPTOMOPY_H_
#define MANTID_ALGORITHMS_TOMOGRAPHY_FBPTOMOPY_H_

#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {
namespace Tomography {

/**
  Wrapper for C function(s) that implement tomopy reconstruction
  methods.

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

/// FBP - Filtered Back-Projection routine as implemented in tomopy
void MANTID_ALGORITHMS_DLL FBPTomopy(const float *data, int dy, int dt, int dx,
                                     const float *center, const float *theta,
                                     float *recon, int ngridx, int ngridy);

} // namespace Tomography
} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_TOMOGRAPHY_FBPTOMOPY_H_ */
