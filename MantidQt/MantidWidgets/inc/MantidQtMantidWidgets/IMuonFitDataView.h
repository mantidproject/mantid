#ifndef MANTID_MANTIDWIDGETS_IMUONFITDATAVIEW_H_
#define MANTID_MANTIDWIDGETS_IMUONFITDATAVIEW_H_

#include "WidgetDllOption.h"
#include <QString>
#include <QStringList>

namespace MantidQt {
namespace MantidWidgets {

/**
 * Interface for MuonFitDataView
 *
 * This abstract base class is used for mocking purposes
 */
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS IMuonFitDataView {
public:
  virtual QStringList getRuns() const = 0;
  virtual unsigned int getWorkspaceIndex() const = 0;
  virtual void setWorkspaceIndex(unsigned int index) = 0;
  virtual double getStartTime() const = 0;
  virtual void setStartTime(double start) = 0;
  virtual double getEndTime() const = 0;
  virtual void setEndTime(double end) = 0;
  virtual void setPeriodVisibility(bool visible) = 0;
  virtual void addGroupCheckbox(const QString &name) = 0;
  virtual void clearGroupCheckboxes() = 0;
  virtual bool isGroupSelected(const QString &name) const = 0;
  virtual void setGroupSelected(const QString &name, bool selected) = 0;
  virtual void setNumPeriods(size_t numPeriods) = 0;
  virtual QStringList getPeriodSelections() const = 0;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTID_MANTIDWIDGETS_IMUONFITDATAVIEW_H_ */