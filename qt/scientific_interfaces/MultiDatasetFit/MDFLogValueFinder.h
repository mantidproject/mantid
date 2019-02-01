// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_MDFLOGVALUEFINDER_H_
#define MANTIDQT_CUSTOMINTERFACES_MDFLOGVALUEFINDER_H_

#include "DllConfig.h"
#include "MantidKernel/Statistics.h"
#include <QStringList>

namespace MantidQt {
namespace CustomInterfaces {

/** MDFLogValueFinder : Holds a collection of workspace names, and finds log
  values from them
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