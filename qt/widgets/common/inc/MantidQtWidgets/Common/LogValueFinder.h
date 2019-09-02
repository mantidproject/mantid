// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_MANTIDWIDGETS_MDFLOGVALUEFINDER_H_
#define MANTIDQT_MANTIDWIDGETS_MDFLOGVALUEFINDER_H_

#include "DllOption.h"
#include "MantidKernel/Statistics.h"
#include <QStringList>

namespace MantidQt {
namespace MantidWidgets {

/** MDFLogValueFinder : Holds a collection of workspace names, and finds log
  values from them
*/
class EXPORT_OPT_MANTIDQT_COMMON LogValueFinder {
public:
  /// Constructor
  explicit LogValueFinder(const QStringList &wsNames);
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

} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTIDQT_MANTIDWIDGETS_MDFLOGVALUEFINDER_H_ */