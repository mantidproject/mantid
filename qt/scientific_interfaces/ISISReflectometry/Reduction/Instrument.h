#ifndef MANTID_CUSTOMINTERFACES_INSTRUMENT_H_
#define MANTID_CUSTOMINTERFACES_INSTRUMENT_H_
/**
Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#include "../DllConfig.h"
#include "DetectorCorrections.h"
#include "MonitorCorrections.h"
#include "RangeInLambda.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL Instrument {
public:
  Instrument(RangeInLambda wavelengthRange,
             MonitorCorrections monitorCorrections,
             DetectorCorrections detectorCorrections);

  bool isValid() const;
  RangeInLambda const &wavelengthRange() const;
  bool integratedMonitors() const;
  size_t monitorIndex() const;
  RangeInLambda monitorIntegralRange() const;
  RangeInLambda monitorBackgroundRange() const;
  bool correctDetectors() const;
  DetectorCorrectionType detectorCorrectionType() const;

private:
  RangeInLambda m_wavelengthRange;
  MonitorCorrections m_monitorCorrections;
  DetectorCorrections m_detectorCorrections;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACES_INSTRUMENT_H_
