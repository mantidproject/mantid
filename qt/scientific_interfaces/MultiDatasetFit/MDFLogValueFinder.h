#ifndef MANTIDQT_CUSTOMINTERFACES_MDFLOGVALUEFINDER_H_
#define MANTIDQT_CUSTOMINTERFACES_MDFLOGVALUEFINDER_H_

#include "DllConfig.h"
#include "MantidKernel/Statistics.h"
#include <QStringList>

namespace MantidQt {
namespace CustomInterfaces {

/** MDFLogValueFinder : Holds a collection of workspace names, and finds log
  values from them

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
class MANTIDQT_MULTIDATASETFIT_DLL MDFLogValueFinder {
public:
  /// Constructor
  explicit MDFLogValueFinder(const QStringList &wsNames);
  /// Get log names from workspaces
  std::vector<std::string> getLogNames() const;
  /// Get log value from workspace position in list
  double getLogValue(const QString &logName,
                     const Mantid::Kernel::Math::StatisticType &function,
                     int index) const;
  /// Get log value from workspace name
  double getLogValue(const QString &logName,
                     const Mantid::Kernel::Math::StatisticType &function,
                     const QString &wsName) const;

private:
  /// Workspace names
  const QStringList m_wsNames;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQT_CUSTOMINTERFACES_MDFLOGVALUEFINDER_H_ */