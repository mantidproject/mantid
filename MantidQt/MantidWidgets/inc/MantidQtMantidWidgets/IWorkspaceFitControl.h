#ifndef MANTID_MANTIDWIDGETS_IWORKSPACEFITCONTROL_H_
#define MANTID_MANTIDWIDGETS_IWORKSPACEFITCONTROL_H_

#include "WidgetDllOption.h"
#include <QString>

namespace MantidQt {
namespace MantidWidgets {

/** IWorkspaceFitControl: set data to fit for a fit property browser

  Abstract base class to be implemented

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS IWorkspaceFitControl {
public:
  virtual ~IWorkspaceFitControl() {}
  virtual void setWorkspaceName(const QString &wsName) = 0;
  virtual void setStartX(double start) = 0;
  virtual void setEndX(double end) = 0;
  virtual void setWorkspaceIndex(int i) = 0;
  virtual void allowSequentialFits(bool allow) = 0;
  virtual bool rawData() const = 0;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTID_MANTIDWIDGETS_IWORKSPACEFITCONTROL_H_