#ifndef MANTID_MANTIDWIDGETS_TRACKEDACTION_H_
#define MANTID_MANTIDWIDGETS_TRACKEDACTION_H_

#include "WidgetDllOption.h"
#include <QAction>

namespace MantidQt {
namespace MantidWidgets {

/** TrackedAction : This is a version of QAction that tracks usage through the
  Mantid usage service

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
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS TrackedAction : public QAction {
  Q_OBJECT
public:
  TrackedAction(QObject *parent);
  TrackedAction(const QString &text, QObject *parent);
  TrackedAction(const QIcon &icon, const QString &text, QObject *parent);
  virtual ~TrackedAction() = default;

  void setTrackingName(const std::string &name);
  std::string getTrackingName() const;

  void setIsTracking(const bool enableTracking);
  bool getIsTracking() const;

protected:
  virtual std::string generateTrackingName() const;
  virtual void registerUsage(const std::string &name);

private:
  void setupTracking();
  bool m_isTracking;
  mutable std::string m_trackingName;

public slots:
  void trackActivation(const bool checked);
};

} // namespace MantidWidgets
} // namespace Mantid

#endif /* MANTID_MANTIDWIDGETS_TRACKEDACTION_H_ */