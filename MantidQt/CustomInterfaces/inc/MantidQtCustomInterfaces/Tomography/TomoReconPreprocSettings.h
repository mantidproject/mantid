#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMORECONPREPROCSETTINGS_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMORECONPREPROCSETTINGS_H_

#include "MantidQtCustomInterfaces/DllConfig.h"

#include <cstddef>

namespace MantidQt {
namespace CustomInterfaces {

/**
Settings for pre-processing of the raw input images before
reconstructing a volume. Note this is in principle not tied to any
particular tool.

Copyright &copy; 2015,2016 ISIS Rutherford Appleton Laboratory, NScD
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
struct MANTIDQT_CUSTOMINTERFACES_DLL TomoReconPreprocSettings {
  bool normalizeByAirRegion;
  bool normalizeByProtonCharge;
  bool normalizeByFlats;
  bool normalizeByDarks;

  // block-size in pixels
  size_t medianFilterWidth;
  // rotation to correct raw images, in degrees. Only multiples of 90
  int rotation;
  // maximum angle, in degrees
  double maxAngle;
  // block size in pixels
  size_t scaleDownFactor;

  TomoReconPreprocSettings();
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMORECONPREPROCSETTINGS_H_
