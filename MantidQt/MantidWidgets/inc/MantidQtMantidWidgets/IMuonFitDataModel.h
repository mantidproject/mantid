#ifndef MANTID_MANTIDWIDGETS_IMUONFITDATAMODEL_H_
#define MANTID_MANTIDWIDGETS_IMUONFITDATAMODEL_H_

#include "WidgetDllOption.h"
#include <QObject>
#include <QString>
#include <QStringList>

namespace MantidQt {
namespace MantidWidgets {

/** IMuonFitDataModel: set data to fit for a muon fit property browser

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
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS IMuonFitDataModel {
public:
  virtual void setWorkspaceNames(const QStringList &wsNames) = 0;
  virtual void workspacesToFitChanged(int n) = 0;
  virtual void setSimultaneousLabel(const std::string &label) = 0;
  virtual void userChangedDataset(int index) = 0;
  virtual void continueAfterChecks(bool sequential) = 0;
signals:
  virtual void preFitChecksRequested(bool sequential) = 0;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTID_MANTIDWIDGETS_IMUONFITDATAMODEL_H