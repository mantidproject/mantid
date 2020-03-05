// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MANTIDWIDGETS_TRACKEDACTION_H_
#define MANTID_MANTIDWIDGETS_TRACKEDACTION_H_

#include "DllOption.h"
#include <QAction>

namespace MantidQt {
namespace MantidWidgets {

/** TrackedAction : This is a version of QAction that tracks usage through the
  Mantid usage service
*/
class EXPORT_OPT_MANTIDQT_COMMON TrackedAction : public QAction {
  Q_OBJECT
public:
  TrackedAction(QObject *parent);
  TrackedAction(const QString &text, QObject *parent);
  TrackedAction(const QIcon &icon, const QString &text, QObject *parent);
  virtual ~TrackedAction() = default;

  void setTrackingName(const std::vector<std::string> &name);
  std::vector<std::string> getTrackingName() const;

  void setIsTracking(const bool enableTracking);
  bool getIsTracking() const;

protected:
  virtual std::vector<std::string> generateTrackingName() const;
  virtual void registerUsage(const std::vector<std::string> &name);

private:
  void setupTracking();
  bool m_isTracking;
  mutable std::vector<std::string> m_trackingName;

public slots:
  void trackActivation(const bool checked);
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTID_MANTIDWIDGETS_TRACKEDACTION_H_ */