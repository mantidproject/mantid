#ifndef MANTIDQT_CUSTOMINTERFACES_MEASUREMENTITEM_H_
#define MANTIDQT_CUSTOMINTERFACES_MEASUREMENTITEM_H_

#include "DllConfig.h"
#include <string>

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
class MANTIDQT_ISISREFLECTOMETRY_DLL MeasurementItem {

public:
  using IDType = std::string;

  /// Constructor
  MeasurementItem(const IDType &measurementItemId, const IDType &subId,
                  const std::string &label, const std::string &type,
                  const double angle, const std::string &run,
                  const std::string &title);

  /// Constructional method
  static MeasurementItem InvalidMeasurementItem(const std::string &why);

  /// Copy constructor
  MeasurementItem(const MeasurementItem &other);

  /// Destructor
  ~MeasurementItem();

  bool isUseable() const;
  std::string whyUnuseable() const;
  IDType id() const;
  IDType subId() const;
  std::string run() const;
  std::string type() const;
  std::string title() const;
  std::string label() const;
  double angle() const;
  std::string angleStr() const;
  MeasurementItem &operator=(const MeasurementItem &);

private:
  /// Constructor
  MeasurementItem(const std::string &why);
  IDType m_measurementItemId;
  IDType m_subId;
  std::string m_label;
  std::string m_type;
  double m_angle;
  std::string m_run;
  std::string m_title;
  std::string m_whyUnuseable;
  /// Not assignable
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQT_CUSTOMINTERFACES_MEASUREMENTITEM_H_ */
