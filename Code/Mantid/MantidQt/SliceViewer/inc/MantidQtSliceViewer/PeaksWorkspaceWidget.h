#ifndef PEAKSWORKSPACEWIDGET_H
#define PEAKSWORKSPACEWIDGET_H

#include <QtGui/QWidget>
#include "DllOption.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "ui_PeaksWorkspaceWidget.h"

namespace MantidQt
{
namespace SliceViewer
{

  class EXPORT_OPT_MANTIDQT_SLICEVIEWER PeaksWorkspaceWidget : public QWidget
  {
    Q_OBJECT
  public:
    PeaksWorkspaceWidget(Mantid::API::IPeaksWorkspace_const_sptr ws, const std::string& coordinateSystem, const QColor& defaultForegroundColour, const QColor& defaultBackgroundColour, QWidget *parent = 0);
    ~PeaksWorkspaceWidget();
  signals:
    void peakColourChanged(Mantid::API::IPeaksWorkspace_const_sptr, QColor);
    void backgroundColourChanged(Mantid::API::IPeaksWorkspace_const_sptr, QColor);
    void backgroundRadiusShown(Mantid::API::IPeaksWorkspace_const_sptr, bool);
  private:
    /// Populate the widget with model data.
    void populate();
    /// Auto-generated UI controls.
    Ui::PeaksWorkspaceWidget ui;
    /// Peaks workspace to view.
    Mantid::API::IPeaksWorkspace_const_sptr m_ws;
    /// Coordinate system.
    const std::string m_coordinateSystem;
    /// Foreground colour
    QColor m_foregroundColour;
    /// Background colour
    QColor m_backgroundColour;

  private slots:
      void onExpandChanged(bool);
      void onBackgroundColourClicked();
      void onForegroundColourClicked();
      void onShowBackgroundChanged(bool);
  };

} //namespace
}
#endif // PEAKSWORKSPACEWIDGET_H
