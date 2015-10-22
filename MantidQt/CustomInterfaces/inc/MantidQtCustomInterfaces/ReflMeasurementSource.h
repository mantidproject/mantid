#ifndef MANTIDQT_CUSTOMINTERFACES_REFLMEASUREMENTSOURCE_H_
#define MANTIDQT_CUSTOMINTERFACES_REFLMEASUREMENTSOURCE_H_

#include "MantidQtCustomInterfaces/DllConfig.h"
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

/**
 * Measurement block
 */
struct Measurement {
  std::string measurementId;
  std::string subId;
  std::string label;
  std::string type;
};

/** ReflMeasurementSource : Repository pattern abstracting data mapping from
 domain. Specifically for accessing
 * measurement information from some data map/repository.

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
class MANTIDQT_CUSTOMINTERFACES_DLL ReflMeasurementSource {
public:
  /// Get the measurement somehow using location
  virtual Measurement obtain(const std::string &location) const = 0;
  /// Virtual destructor
  virtual ReflMeasurementSource *clone() const = 0;
  /// Destructor
  virtual ~ReflMeasurementSource(){};
};

} // namespace CustomInterfaces
} // namespace Mantid

#endif /* MANTIDQT_CUSTOMINTERFACES_REFLMEASUREMENTSOURCE_H_ */
