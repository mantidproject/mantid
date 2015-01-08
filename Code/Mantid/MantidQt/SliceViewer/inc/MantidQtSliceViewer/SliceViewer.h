#ifndef SLICEVIEWER_H
#define SLICEVIEWER_H

#include "ColorBarWidget.h"
#include "DimensionSliceWidget.h"
#include "DllOption.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/PeakTransformSelector.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/VMD.h"
#include "MantidQtAPI/MantidColorMap.h"
#include "MantidQtAPI/MdSettings.h"
#include "MantidQtMantidWidgets/SafeQwtPlot.h"
#include "MantidQtAPI/SyncedCheckboxes.h"
#include "MantidQtSliceViewer/LineOverlay.h"
#include "MantidQtSliceViewer/PeaksPresenter.h"
#include "MantidQtSliceViewer/ZoomablePeaksView.h"
#include "MantidQtAPI/QwtRasterDataMD.h"
#include "ui_SliceViewer.h"
#include <QtCore/QtCore>
#include <QtGui/qdialog.h>
#include <QtGui/QWidget>
#include <qwt_color_map.h>
#include <qwt_plot_spectrogram.h>
#include <qwt_plot.h>
#include <qwt_raster_data.h>
#include <qwt_scale_widget.h>
#include <vector>
#include <Poco/NObserver.h>
#include "MantidAPI/Algorithm.h"
#include "MantidQtAPI/AlgorithmRunner.h"

namespace MantidQt
{
namespace SliceViewer
{

// Forward dec
class CompositePeaksPresenter;
class ProxyCompositePeaksPresenter;

/** GUI for viewing a 2D slice out of a multi-dimensional workspace.
 * You can select which dimension to plot as X,Y, and the cut point
 * along the other dimension(s).
 *
 */
class EXPORT_OPT_MANTIDQT_SLICEVIEWER SliceViewer : public QWidget, public ZoomablePeaksView
{
  friend class SliceViewerWindow;

  Q_OBJECT

public:
  SliceViewer(QWidget *parent = 0);
  ~SliceViewer();

  void setWorkspace(const QString & wsName);
  void setWorkspace(Mantid::API::IMDWorkspace_sptr ws);
  Mantid::API::IMDWorkspace_sptr getWorkspace();
  void showControls(bool visible);
  void zoomBy(double factor);
  void loadColorMap(QString filename = QString() );
  LineOverlay * getLineOverlay() { return m_lineOverlay; }
  Mantid::Kernel::VMD getSlicePoint() const { return m_slicePoint; }
  int getDimX() const;
  int getDimY() const;

  /// Methods for Python bindings
  QString getWorkspaceName() const;
  void setXYDim(int indexX, int indexY);
  void setXYDim(const QString & dimX, const QString & dimY);
  void setSlicePoint(int dim, double value);
  void setSlicePoint(const QString & dim, double value);
  double getSlicePoint(int dim) const;
  double getSlicePoint(const QString & dim) const;
  void setColorScaleMin(double min);
  void setColorScaleMax(double max);
  void setColorScaleLog(bool log);
  void setColorScale(double min, double max, bool log);
  void setColorMapBackground(int r, int g, int b);
  double getColorScaleMin() const;
  double getColorScaleMax() const;
  bool getColorScaleLog() const;
  bool getFastRender() const;
  void setXYLimits(double xleft, double xright, double ybottom, double ytop);
  QwtDoubleInterval getXLimits() const;
  QwtDoubleInterval getYLimits() const;
  void setXYCenter(double x, double y);
  void openFromXML(const QString & xml);
  void toggleLineMode(bool);
  void setNormalization(Mantid::API::MDNormalization norm, bool update=true);
  Mantid::API::MDNormalization getNormalization() const;

  /// Dynamic Rebinning-related Python bindings
  void setRebinThickness(int dim, double thickness);
  void setRebinNumBins(int xBins, int yBins);
  void setRebinMode(bool mode, bool locked);
  void refreshRebin();

  /// Methods relating to peaks overlays.
  boost::shared_ptr<ProxyCompositePeaksPresenter> getPeaksPresenter() const;
  ProxyCompositePeaksPresenter* setPeaksWorkspaces(const QStringList& list); // For python binding
  void clearPeaksWorkspaces(); // For python binding

  /* -- Methods from implementation of ZoomablePeaksView. --*/
  virtual void zoomToRectangle(const PeakBoundingBox& box);
  virtual void resetView();

signals:
  /// Signal emitted when the X/Y index of the shown dimensions is changed
  void changedShownDim(size_t dimX, size_t dimY);
  /// Signal emitted when the slice point moves
  void changedSlicePoint(Mantid::Kernel::VMD slicePoint);
  /// Signal emitted when the LineViewer should be shown/hidden.
  void showLineViewer(bool);
  /// Signal emitted when the PeaksViewer should be shown/hidden.
  void showPeaksViewer(bool);
  /// Signal emitted when someone uses setWorkspace() on SliceViewer
  void workspaceChanged();
  /// Signal emitted when someone wants to see the options dialog
  void peaksTableColumnOptions();

public slots:
  void helpSliceViewer();
  void helpLineViewer();
  void helpPeaksViewer();
  void setFastRender(bool fast);
  void showInfoAt(double, double);

  // Change in view slots
  void changedShownDim(int index, int dim, int oldDim);
  void updateDisplaySlot(int index, double value);
  void resetZoom();
  void setXYLimitsDialog();
  void zoomInSlot();
  void zoomOutSlot();
  void zoomRectSlot(const QwtDoubleRect & rect);
  void panned(int, int);
  void magnifierRescaled(double);

  // Color scale slots
  void setColorScaleAutoFull();
  void setColorScaleAutoSlice();
  void colorRangeChanged();
  void loadColorMapSlot();
  void setTransparentZeros(bool transparent);
  void changeNormalizationNone();
  void changeNormalizationVolume();
  void changeNormalizationNumEvents();

  // Buttons or actions
  void clearLine();
  QPixmap getImage();
  void saveImage(const QString & filename = QString());
  void copyImageToClipboard();
  void onPeaksViewerOverlayOptions();

  // Synced checkboxes
  void LineMode_toggled(bool);
  void SnapToGrid_toggled(bool);
  void RebinMode_toggled(bool);
  void RebinLock_toggled(bool);
  void autoRebin_toggled(bool);

  // Dynamic rebinning
  void rebinParamsChanged();
  void dynamicRebinComplete(bool error);

  // Peaks overlay
  void peakOverlay_toggled(bool);

private:
  void loadSettings();
  void saveSettings();
  void initMenus();
  void initZoomer();

  void updateDisplay(bool resetAxes = false);
  void updateDimensionSliceWidgets();
  void resetAxis(int axis, const Mantid::Geometry::IMDDimension_const_sptr & dim);

  void findRangeFull();
  void findRangeSlice();

  // Peak overlay methods.
  void updatePeakOverlaySliderWidget();
  void updatePeaksOverlay();
  void enablePeakOverlaysIfAppropriate();
  void disablePeakOverlays();

  // Autorebin methods.
  bool isAutoRebinSet() const;
  void autoRebinIfRequired();

  // helper for saveImage
  QString ensurePngExtension(const QString& fname) const;

private:

  // -------------------------- Widgets ----------------------------

  /// Auto-generated UI controls.
  Ui::SliceViewerClass ui;

  /// Main plot object
  MantidQt::MantidWidgets::SafeQwtPlot * m_plot;

  /// Spectrogram plot
  QwtPlotSpectrogram * m_spect;

  /// Layout containing the spectrogram
  QHBoxLayout * m_spectLayout;

  /// Color bar indicating the color scale
  ColorBarWidget * m_colorBar;

  /// Vector of the widgets for slicing dimensions
  std::vector<DimensionSliceWidget *> m_dimWidgets;

  /// The LineOverlay widget for drawing line cross-sections (hidden at startup)
  LineOverlay * m_lineOverlay;

  /// The LineOverlay widget for drawing the outline of the rebinned workspace
  LineOverlay * m_overlayWSOutline;

  //PeakOverlay * m_peakOverlay;

  /// Object for running algorithms in the background
  MantidQt::API::AlgorithmRunner * m_algoRunner;

  // -------------------------- Data Members ----------------------------

  /// Workspace being shown
  Mantid::API::IMDWorkspace_sptr m_ws;

  /// Workspace overlaid on top of original (optional) for dynamic rebinning
  Mantid::API::IMDWorkspace_sptr m_overlayWS;

  /// Set to true once the first workspace has been loaded in it
  bool m_firstWorkspaceOpen;

  /// File of the last loaded color map.
  QString m_currentColorMapFile;

  /// Vector of the dimensions to show.
  std::vector<Mantid::Geometry::MDHistoDimension_sptr> m_dimensions;

  /// Data presenter
  API::QwtRasterDataMD * m_data;

  /// The X and Y dimensions being plotted
  Mantid::Geometry::IMDDimension_const_sptr m_X;
  Mantid::Geometry::IMDDimension_const_sptr m_Y;
  size_t m_dimX;
  size_t m_dimY;

  /// The point of slicing in the other dimensions
  Mantid::Kernel::VMD m_slicePoint;

  /// The range of values to fit in the color map.
  QwtDoubleInterval m_colorRange;

  /// The calculated range of values in the FULL data set
  QwtDoubleInterval m_colorRangeFull;

  /// The calculated range of values ONLY in the currently viewed part of the slice
  QwtDoubleInterval m_colorRangeSlice;

  /// Use the log of the value for the color scale
  bool m_logColor;

  /// Menus
  QMenu *m_menuColorOptions, *m_menuView, *m_menuHelp, *m_menuLine, *m_menuFile, *m_menuPeaks;
  QAction *m_actionFileClose;
  QAction *m_actionTransparentZeros;
  QAction *m_actionNormalizeNone;
  QAction *m_actionNormalizeVolume;
  QAction *m_actionNormalizeNumEvents;
  QAction *m_actionRefreshRebin;

  /// Synced menu/buttons
  MantidQt::API::SyncedCheckboxes *m_syncLineMode, *m_syncSnapToGrid,
    *m_syncRebinMode, *m_syncRebinLock, *m_syncPeakOverlay, *m_syncAutoRebin;

  /// Cached double for infinity
  double m_inf;

  /// "Fast" rendering mode
  bool m_fastRender;

  /// Last path that was saved using saveImage()
  QString m_lastSavedFile;

  /// Name of the workspace generated by the dynamic rebinning BinMD call
  std::string m_overlayWSName;

  /// If true, then we are in dynamic rebin mode
  bool m_rebinMode;

  /// If true, the rebinned overlayWS is locked until refreshed.
  bool m_rebinLocked;

  /// Md Settings for color maps 
  boost::shared_ptr<MantidQt::API::MdSettings>  m_mdSettings;

  // -------------------------- Controllers ------------------------
  boost::shared_ptr<CompositePeaksPresenter>  m_peaksPresenter;

  boost::shared_ptr<ProxyCompositePeaksPresenter> m_proxyPeaksPresenter;

  /// Pointer to widget used for peaks sliding.
  DimensionSliceWidget* m_peaksSliderWidget;

  /// Object for choosing a PeakTransformFactory based on the workspace type.
  Mantid::API::PeakTransformSelector m_peakTransformSelector;
};

} // namespace SliceViewer
} // namespace Mantid

#endif // SLICEVIEWER_H
