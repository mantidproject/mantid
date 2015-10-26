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
  typedef const std::string IDType;

  /// Constructor
  Measurement(const IDType &measurementId, const IDType &subId,
              const std::string &label, const std::string &type,
              const double angle, const std::string &run);

  /// Constructional method
  static Measurement InvalidMeasurement(const std::string& why);

  /// Copy constructor
  Measurement(const Measurement &other);

  /// Destructor
  ~Measurement();

  bool isUseable() const;
  std::string whyUnuseable() const;
  IDType id() const;
  IDType subId() const;
  std::string run() const;
  std::string type() const;
  std::string label() const;
  double angle() const;
  std::string angleStr() const;

private:
  /// Constructor
  Measurement(const std::string& why);
  const IDType m_measurementId;
  const IDType m_subId;
  const std::string m_label;
  const std::string m_type;
  const double m_angle;
  const std::string m_run;
  std::string m_whyUnuseable;
  /// Not assignable
  Measurement &operator=(const Measurement &);
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQT_CUSTOMINTERFACES_MEASUREMENT_H_ */
