#ifndef VSI_PEAKSTABWIDGET_H
#define VSI_PEAKSTABWIDGET_H

#include "ui_PeaksTabWidget.h"
#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"
#include "MantidAPI/IPeaksWorkspace.h"

#include <QWidget>
#include <string>
#include <vector>
#include <map>

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{
  class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS PeaksTabWidget : public QWidget
  {
    Q_OBJECT
  public:
    PeaksTabWidget(std::vector<Mantid::API::IPeaksWorkspace_sptr> ws, const std::string& coordinateSystem, QWidget *parent = 0);
    ~PeaksTabWidget();
    void setupMvc(std::map<std::string, std::vector<bool>> visiblePeaks);
    void addNewPeaksWorkspace(Mantid::API::IPeaksWorkspace_sptr peaksWorkspace, std::vector<bool> visiblePeaks);
    void updateTabs(std::map<std::string, std::vector<bool>> visiblePeaks);
  signals:
    void zoomToPeak(Mantid::API::IPeaksWorkspace_sptr ws, int row);
  public slots:
    void onZoomToPeak(Mantid::API::IPeaksWorkspace_sptr ws, int row);
  private:
    /// Update a certain tab.
    void updateTab(std::vector<bool> visiblePeaks, int index);
    /// Adds a new tab to the tab widget.
    void PeaksTabWidget::addNewTab(Mantid::API::IPeaksWorkspace_sptr peaksWorkspace, std::string tabName, std::vector<bool> visiblePeaks);
    /// Auto-generated UI controls.
    Ui::PeaksTabWidget ui;
    /// Peaks workspace to view.
    std::vector<Mantid::API::IPeaksWorkspace_sptr> m_ws;
    /// Coordinate system.
    const std::string m_coordinateSystem;
  };
} //namespace
}
}
#endif // PEAKSWORKSPACEWIDGET_H
