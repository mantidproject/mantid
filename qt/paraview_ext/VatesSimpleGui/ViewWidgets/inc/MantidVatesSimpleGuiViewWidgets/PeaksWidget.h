#ifndef VSI_PEAKSWORKWIDGET_H
#define VSI_PEAKSWORKWIDGET_H

#include "MantidAPI/IPeaksWorkspace_fwd.h"
#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"
#include "ui_PeaksWidget.h"

#include <QWidget>
#include <map>
#include <string>
#include <vector>

namespace Mantid {
namespace Vates {
namespace SimpleGui {
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS PeaksWidget
    : public QWidget {
  Q_OBJECT
public:
  PeaksWidget(Mantid::API::IPeaksWorkspace_sptr ws,
              const std::string &coordinateSystem, QWidget *parent = nullptr);
  void setupMvc(std::vector<bool> visiblePeaks);
  void updateModel(std::vector<bool> visiblePeaks);
signals:
  void zoomToPeak(Mantid::API::IPeaksWorkspace_sptr ws, int row);
  void sortPeaks(const std::string &columnToSortBy, const bool sortAscending,
                 Mantid::API::IPeaksWorkspace_sptr ws);
public slots:
  void onCurrentChanged(const QModelIndex &current, const QModelIndex &);
  void onPeaksSorted(const std::string &columnToSortBy,
                     const bool sortAscending);

private:
  /// Auto-generated UI controls.
  Ui::PeaksWidget ui;
  /// Peaks workspace to view.
  Mantid::API::IPeaksWorkspace_sptr m_ws;
  /// Coordinate system.
  const std::string m_coordinateSystem;
  /// Table width
  int m_originalTableWidth;
};

} // namespace SimpleGui
} // namespace Vates
} // namespace Mantid
#endif // PEAKSWORKSPACEWIDGET_H
