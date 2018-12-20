#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFCALIBSETTINGS_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFCALIBSETTINGS_H_

#include <string>

namespace MantidQt {
namespace CustomInterfaces {

/**
Settings related to calibration in the engineering diffraction custom
interface. This is just a data holder.

Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD
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
struct EnggDiffCalibSettings {
  std::string m_inputDirCalib;
  std::string m_inputDirRaw;
  std::string m_pixelCalibFilename;
  bool m_forceRecalcOverwrite;
  std::string m_templateGSAS_PRM;
  float m_rebinCalibrate;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFCALIBSETTINGS_H_
