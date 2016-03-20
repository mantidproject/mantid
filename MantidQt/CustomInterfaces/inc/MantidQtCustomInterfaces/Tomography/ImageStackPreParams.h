#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IMAGESTACKPREPARAMS_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IMAGESTACKPREPARAMS_H_

#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidKernel/V2D.h"

#include <utility>

namespace MantidQt {
namespace CustomInterfaces {

/**
This holds parameters for pre-processing images or stacks of images
for tomographic reconstruction. These parameters are used in different
pre-processing steps in the tomographic reconstruction pipeline, and
also for the reconstruction algorithms (iternative methods, FBP,
etc.).

The parameters include:
- center of rotation
- region of interest (clip from original or raw images)
- region for normalization (where the beam is not blocked by any sample 
  object throughout the stack of images) other parameters describing
  the stack of images: 

Copyright &copy; 2014,2015 ISIS Rutherford Appleton Laboratory, NScD
Oak Ridge National Laboratory & European Spallation Source

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
class MANTIDQT_CUSTOMINTERFACES_DLL ImageStackPreParams {
public:

  ImageStackPreParams();

  typedef std::pair<Mantid::Kernel::V2D, Mantid::Kernel::V2D> Box2D;

  Mantid::Kernel::V2D cor;
  Box2D roi;
  Box2D normalizationRegion;  //< also known as 'air' region
  float rotation; //< rotation angle in degrees
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IMAGESTACKPREPARAMS_H_
