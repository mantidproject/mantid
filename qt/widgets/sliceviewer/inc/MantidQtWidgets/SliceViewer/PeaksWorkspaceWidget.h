#ifndef PEAKSWORKSPACEWIDGET_H
#define PEAKSWORKSPACEWIDGET_H

#include "DllOption.h"
#include "MantidAPI/IPeaksWorkspace_fwd.h"
#include "MantidQtWidgets/SliceViewer/PeakViewColor.h"
#include "ui_PeaksWorkspaceWidget.h"
#include <QSortFilterProxyModel>
#include <QWidget>

#include <set>

namespace MantidQt {
namespace SliceViewer {
class PeaksViewer;
class EXPORT_OPT_MANTIDQT_SLICEVIEWER PeaksWorkspaceWidget : public QWidget {
  Q_OBJECT
public:
  PeaksWorkspaceWidget(Mantid::API::IPeaksWorkspace_const_sptr ws,
                       const std::string &coordinateSystem,
                       PeakViewColor defaultForegroundPeakViewColor,
                       PeakViewColor defaultBackgroundPeakViewColor,
                       PeaksViewer *parent);

  std::set<QString> getShownColumns();
  void setShownColumns(const std::set<QString> &cols);
  ~PeaksWorkspaceWidget() override;
  Mantid::API::IPeaksWorkspace_const_sptr getPeaksWorkspace() const;
  void setBackgroundColor(const PeakViewColor &backgroundColor);
  void setForegroundColor(const PeakViewColor &foregroundColor);
  void setShowBackground(bool showBackground);
  void setHidden(bool isHidden);
  void setSelectedPeak(int index);
  std::string getWSName() const;
  void workspaceUpdate(Mantid::API::IPeaksWorkspace_const_sptr ws =
                           Mantid::API::IPeaksWorkspace_const_sptr());
  void exitClearPeaksMode();
  void exitAddPeaksMode();
signals:
  void peakColourChanged(Mantid::API::IPeaksWorkspace_const_sptr, QColor);
  void peakColorchanged(Mantid::API::IPeaksWorkspace_const_sptr, PeakViewColor);
  void backgroundColourChanged(Mantid::API::IPeaksWorkspace_const_sptr, QColor);
  void backgroundColorChanged(Mantid::API::IPeaksWorkspace_const_sptr,
                              PeakViewColor);
  void backgroundRadiusShown(Mantid::API::IPeaksWorkspace_const_sptr, bool);
  void removeWorkspace(Mantid::API::IPeaksWorkspace_const_sptr);
  void hideInPlot(Mantid::API::IPeaksWorkspace_const_sptr, bool);
  void zoomToPeak(Mantid::API::IPeaksWorkspace_const_sptr, int);

private:
  /// Populate the widget with model data.
  void populate();
  /// Create the MVC table for peaks display
  void createTableMVC();
  /// Sets up the background peak view colors
  void onBackgroundPeakViewColorClicked();
  /// Sets up the foreground peak view colors
  void onForegroundPeakViewColorClicked();
  /// Auto-generated UI controls.
  Ui::PeaksWorkspaceWidget ui;
  /// Peaks workspace to view.
  Mantid::API::IPeaksWorkspace_const_sptr m_ws;
  /// Coordinate system.
  const std::string m_coordinateSystem;
  /// Foreground PeakViewColor
  PeakViewColor m_foregroundPeakViewColor;
  /// Background PeakViewColor
  PeakViewColor m_backgroundPeakViewColor;
  /// Original table width
  int m_originalTableWidth;
  /// Workspace name.
  QString m_nameText;
  /// Parent widget
  PeaksViewer *const m_parent;
  /// Sort & Filter model to map sorted indicies to unsorted indicies
  QSortFilterProxyModel *m_tableModel;

private slots:
  void onForegroundColorCrossClicked();
  void onBackgroundColorSphereClicked();
  void onForegroundColorSphereClicked();
  void onBackgroundColorEllipsoidClicked();
  void onForegroundColorEllipsoidClicked();

  void onShowBackgroundChanged(bool);
  void onRemoveWorkspaceClicked();
  void onToggleHideInPlot();
  void onCurrentChanged(QModelIndex, QModelIndex);
  void onClearPeaksToggled(bool);
  void onAddPeaksToggled(bool);
};

} // namespace SliceViewer
} // namespace MantidQt
#endif // PEAKSWORKSPACEWIDGET_H
