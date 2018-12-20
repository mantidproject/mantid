#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONSETTINGS_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONSETTINGS_H_

#include <string>

#include "EnggDiffCalibSettings.h"

namespace MantidQt {
namespace CustomInterfaces {

/**
Interface to settings of the Engineering Diffraction (EnggDiffraction)
GUI. This can be used in different tabs/widgets as well as in the
main/central view. Normally the individual / area specific
tabs/widgets will forward to the main view.

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
class IEnggDiffractionSettings {
public:
  virtual ~IEnggDiffractionSettings() = default;

  /**
   * Calibration settings as defined by the user.
   *
   * @return calibration settings object with current user settings
   */
  virtual EnggDiffCalibSettings currentCalibSettings() const = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONSETTINGS_H_
