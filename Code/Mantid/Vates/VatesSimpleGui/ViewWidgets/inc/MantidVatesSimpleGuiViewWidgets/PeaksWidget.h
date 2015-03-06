#ifndef VSI_PEAKSWORKWIDGET_H
#define VSI_PEAKSWORKWIDGET_H

#include "ui_PeaksWidget.h"
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
  class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS PeaksWidget : public QWidget
  {
    Q_OBJECT
  public:
    PeaksWidget(Mantid::API::IPeaksWorkspace_sptr ws, const std::string& coordinateSystem, QWidget *parent = 0);
    void setupMvc(std::vector<bool> visiblePeaks);
    void updateModel(std::vector<bool> visiblePeaks);
  signals:
    void zoomToPeak(Mantid::API::IPeaksWorkspace_sptr ws, int row);
  public slots:
    void onCurrentChanged(QModelIndex current, QModelIndex);
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

} //namespace
}
}
#endif // PEAKSWORKSPACEWIDGET_H
