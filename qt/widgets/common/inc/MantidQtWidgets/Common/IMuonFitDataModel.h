// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MANTIDWIDGETS_IMUONFITDATAMODEL_H_
#define MANTID_MANTIDWIDGETS_IMUONFITDATAMODEL_H_

#include "DllOption.h"
#include <QObject>
#include <QString>
#include <QStringList>

namespace MantidQt {
namespace MantidWidgets {

/** IMuonFitDataModel: set data to fit for a muon fit property browser

  Abstract base class to be implemented
*/
class EXPORT_OPT_MANTIDQT_COMMON IMuonFitDataModel {
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