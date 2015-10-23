#ifndef MANTIDQT_CUSTOMINTERFACES_MEASUREMENT_H_
#define MANTIDQT_CUSTOMINTERFACES_MEASUREMENT_H_

#include "MantidQtCustomInterfaces/DllConfig.h"
#include <String>

namespace MantidQt {
namespace CustomInterfaces {

/** Measurement : Immutable measurement type

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
class MANTIDQT_CUSTOMINTERFACES_DLL Measurement {

public:
  /// Constructor
  Measurement(const std::string &measurementId, const std::string &subId,
              const std::string &label, const std::string &type);

  /// Constructional method
  static Measurement InvalidMeasurement();

  /// Copy constructor
  Measurement(const Measurement &other);

  /// Destructor
  ~Measurement();

  bool isUseable() const;
  std::string id() const;
  std::string subId() const;
  std::string type() const;
  std::string label() const;

private:
  /// Constructor
  Measurement();
  const std::string m_measurementId;
  const std::string m_subId;
  const std::string m_label;
  const std::string m_type;
  bool m_valid;
  /// Not assignable
  Measurement &operator=(const Measurement &);
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQT_CUSTOMINTERFACES_MEASUREMENT_H_ */
