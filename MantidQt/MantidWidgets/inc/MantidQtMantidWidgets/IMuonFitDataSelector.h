#ifndef MANTID_MANTIDWIDGETS_IMUONFITDATASELECTOR_H_
#define MANTID_MANTIDWIDGETS_IMUONFITDATASELECTOR_H_

#include "WidgetDllOption.h"
#include <QString>
#include <QStringList>

namespace MantidQt {
namespace MantidWidgets {

/**
 * Interface for MuonFitDataSelector
 *
 * This abstract base class can be used for mocking purposes
 */
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS IMuonFitDataSelector {
public:
  virtual ~IMuonFitDataSelector() {}
  virtual QStringList getRuns() const = 0;
  virtual unsigned int getWorkspaceIndex() const = 0;
  virtual double getStartTime() const = 0;
  virtual double getEndTime() const = 0;
  virtual void setNumPeriods(size_t numPeriods) = 0;
  virtual QStringList getPeriodSelections() const = 0;
  virtual void setWorkspaceDetails(int runNumber, const QString &instName) = 0;
  virtual void setAvailableGroups(const QStringList &groupNames) = 0;
  virtual QStringList getChosenGroups() const = 0;
  virtual void setWorkspaceIndex(unsigned int index) = 0;
  virtual void setStartTime(double start) = 0;
  virtual void setEndTime(double end) = 0;
  virtual void setStartTimeQuietly(double start) = 0;
  virtual void setEndTimeQuietly(double end) = 0;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTID_MANTIDWIDGETS_IMUONFITDATASELECTOR_H_ */