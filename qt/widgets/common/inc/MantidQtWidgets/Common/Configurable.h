#ifndef MANTIDQT_MANTIDWIDGETS_CONFIGURABLE_H
#define MANTIDQT_MANTIDWIDGETS_CONFIGURABLE_H
/*
  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
#include "MantidQtWidgets/Common/DllOption.h"

class QSettings;

namespace MantidQt {
namespace MantidWidgets {

/**
 * Defines an interface for an object to load and store
 * any configuration settings that should persist between objects. A widget
 * should inherit from this class and define the loadSettings and saveSettings
 * member functions. These functions are expected to be called by the client
 * along with a QSettings instance, opened at the correct group, which will
 * either give access or receive the values.
 */
class EXPORT_OPT_MANTIDQT_COMMON Configurable {
public:
  virtual ~Configurable() = default;
  virtual void readSettings(const QSettings &) = 0;
  virtual void writeSettings(QSettings &) const = 0;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDQT_MANTIDWIDGETS_CONFIGURABLE_H
