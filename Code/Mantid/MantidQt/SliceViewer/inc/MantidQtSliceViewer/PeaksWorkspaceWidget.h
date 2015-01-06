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
    std::set<QString> getShownColumns();
    void setShownColumns(std::set<QString> & cols);
    virtual ~PeaksWorkspaceWidget();
    Mantid::API::IPeaksWorkspace_const_sptr getPeaksWorkspace() const;
    void setBackgroundColor(const QColor& backgroundColor);
    void setForegroundColor(const QColor& foregroundColor);
    void setShowBackground(bool showBackground);
    void setHidden(bool isHidden);
    void setSelectedPeak(int index);
    std::string getWSName() const;
    void workspaceUpdate(Mantid::API::IPeaksWorkspace_const_sptr ws = Mantid::API::IPeaksWorkspace_const_sptr());
  signals:
    void peakColourChanged(Mantid::API::IPeaksWorkspace_const_sptr, QColor);
    void backgroundColourChanged(Mantid::API::IPeaksWorkspace_const_sptr, QColor);
    void backgroundRadiusShown(Mantid::API::IPeaksWorkspace_const_sptr, bool);
    void removeWorkspace(Mantid::API::IPeaksWorkspace_const_sptr);
    void hideInPlot(Mantid::API::IPeaksWorkspace_const_sptr, bool);
    void zoomToPeak(Mantid::API::IPeaksWorkspace_const_sptr, int);
    void peaksSorted(const std::string&, const bool, Mantid::API::IPeaksWorkspace_const_sptr);
  private:
    /// Populate the widget with model data.
    void populate();
    /// Create the MVC table for peaks display
    void createTableMVC();
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
    /// Original table width
    int m_originalTableWidth;
    /// Workspace name.
    QString m_nameText;

  private slots:
      void onBackgroundColourClicked();
      void onForegroundColourClicked();
      void onShowBackgroundChanged(bool);
      void onRemoveWorkspaceClicked();
      void onToggleHideInPlot();
      void onTableClicked(const QModelIndex&);
      void onPeaksSorted(const std::string&, const bool);
  };

} //namespace
}
#endif // PEAKSWORKSPACEWIDGET_H
