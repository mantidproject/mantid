#ifndef MANTIDQT_CUSTOMINTERFACES_REFLNEXUSMEASUREMENTITEMSOURCE_H_
#define MANTIDQT_CUSTOMINTERFACES_REFLNEXUSMEASUREMENTITEMSOURCE_H_

#include "DllConfig.h"
#include "MeasurementItem.h"
#include "ReflMeasurementItemSource.h"

#include <string>

namespace MantidQt {
namespace CustomInterfaces {

/** ReflNexusMeasurementSource : ReflMeasurementSource repository realization
  that
  fetches data out off disk using load algorithms and Nexus formats.

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTIDQT_ISISREFLECTOMETRY_DLL ReflNexusMeasurementItemSource
    : public ReflMeasurementItemSource {
public:
  ReflNexusMeasurementItemSource();
  MeasurementItem obtain(const std::string &definedPath,
                         const std::string &fuzzyName) const override;
  /// Virtual destructor
  ReflNexusMeasurementItemSource *clone() const override;
  ~ReflNexusMeasurementItemSource() override;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQT_CUSTOMINTERFACES_REFLNEXUSMEASUREMENTSOURCE_H_ */
