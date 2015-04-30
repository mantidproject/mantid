#ifndef VSI_PEAKSTABWIDGET_H
#define VSI_PEAKSTABWIDGET_H

#include "ui_PeaksTabWidget.h"
#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"
#include "MantidAPI/IPeaksWorkspace.h"

#include <QWidget>
#include <QTabWidget>
#include <QTabBar>
#include <string>
#include <vector>
#include <map>

namespace Mantid {
namespace Vates {
namespace SimpleGui {

// Need this class to access color of tabBar
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS PeakCustomTabWidget:public QTabWidget
{
public:
    PeakCustomTabWidget(QWidget* parent = 0)
    {
      setParent(parent);
    }
    
    //Overridden method from QTabWidget
    QTabBar* tabBar()
    {
      return QTabWidget::tabBar();
    }
};

class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS PeaksTabWidget
    : public QWidget {
  Q_OBJECT
public:
  PeaksTabWidget(std::vector<Mantid::API::IPeaksWorkspace_sptr> ws,
                 const std::string &coordinateSystem, QWidget *parent = 0);
  ~PeaksTabWidget();
  void setupMvc(std::map<std::string, std::vector<bool>> visiblePeaks);
  void addNewPeaksWorkspace(Mantid::API::IPeaksWorkspace_sptr peaksWorkspace,
                            std::vector<bool> visiblePeaks);
  void updateTabs(std::map<std::string, std::vector<bool>> visiblePeaks, std::map<std::string, QColor> colors);
signals:
  void zoomToPeak(Mantid::API::IPeaksWorkspace_sptr ws, int row);
  void sortPeaks(const std::string &columnToSortBy, const bool sortAscending,
                 Mantid::API::IPeaksWorkspace_sptr ws);
public slots:
  void onZoomToPeak(Mantid::API::IPeaksWorkspace_sptr ws, int row);

private:
  /// Update a certain tab.
  void updateTab(std::vector<bool> visiblePeaks, QColor color, int index);
  /// Adds a new tab to the tab widget.
  void addNewTab(Mantid::API::IPeaksWorkspace_sptr peaksWorkspace,
                 std::string tabName, std::vector<bool> visiblePeaks);
  /// Auto-generated UI controls.
  Ui::PeaksTabWidget ui;
  /// Peaks workspace to view.
  std::vector<Mantid::API::IPeaksWorkspace_sptr> m_ws;
  /// Coordinate system.
  const std::string m_coordinateSystem;
  /// Custom peaks tab widget
  PeakCustomTabWidget* m_tabWidget;
};
} // namespace
}
}
#endif // PEAKSWORKSPACEWIDGET_H
