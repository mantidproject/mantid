#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONCALIBRATION_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONCALIBRATION_H_

#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

/**
 * Parameters from a GSAS calibration. They define a conversion of
 * units time-of-flight<->d-spacing that can be calculated with the
 * algorithm AlignDetectors for example.
 */
struct GSASCalibrationParms {
  GSASCalibrationParms(size_t bid, double dc, double da, double tz)
      : bankid(bid), difc(dc), difa(da), tzero(tz) {}

  size_t bankid{0};
  double difc{0};
  double difa{0};
  double tzero{0};
};

/**
Interface to the current calibration functionality of the Engineering
Diffraction (EnggDiffraction) GUI. This can be used in different
tabs/widgets as well as in the main/central view. Normally this
interface will be provided by the presenters of the widgets (assuming
an MVP design). The individual / area specific tabs/widgets (their
presenters) will forward to the widget responsible for the
calibration.

Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD
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
class IEnggDiffractionCalibration {
public:
  virtual ~IEnggDiffractionCalibration() = default;

  virtual std::vector<GSASCalibrationParms> currentCalibration() const = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIOPYTHONRUNNER_H_
