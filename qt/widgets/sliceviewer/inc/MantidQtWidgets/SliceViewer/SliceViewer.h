// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef SLICEVIEWER_H
#define SLICEVIEWER_H

#include "DimensionSliceWidget.h"
#include "DllOption.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidGeometry/Crystal/PeakTransformSelector.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/VMD.h"
#include "MantidQtWidgets/Common/AlgorithmRunner.h"
#include "MantidQtWidgets/Common/MdSettings.h"
#include "MantidQtWidgets/Common/SyncedCheckboxes.h"
#include "MantidQtWidgets/Plotting/Qwt/ColorBarWidget.h"
#include "MantidQtWidgets/Plotting/Qwt/QwtRasterDataMD.h"
#include "MantidQtWidgets/Plotting/Qwt/SafeQwtPlot.h"
#include "MantidQtWidgets/SliceViewer/CoordinateTransform.h"
#include "MantidQtWidgets/SliceViewer/LineOverlay.h"
#include "MantidQtWidgets/SliceViewer/NonOrthogonalOverlay.h"
#include "MantidQtWidgets/SliceViewer/PeaksPresenter.h"
#include "MantidQtWidgets/SliceViewer/QwtScaleDrawNonOrthogonal.h"
#include "MantidQtWidgets/SliceViewer/ZoomablePeaksView.h"
#include "ui_SliceViewer.h"
#include <boost/shared_ptr.hpp>
#include <qwt_color_map.h>
#include <qwt_plot.h>
#include <qwt_plot_spectrogram.h>
#include <vector>

class QDragEnterEvent;
class QDropEvent;
class QwtPlotRescaler;

namespace Mantid {
namespace API {
class IPeaksWorkspace;
}
} // namespace Mantid
namespace MantidQt {
namespace SliceViewer {

// Forward dec
class CompositePeaksPresenter;
class ProxyCompositePeaksPresenter;

// Static Const values
static const std::string g_iconPathPrefix = ":/SliceViewer/icons/";
static const std::string g_iconZoomPlus =
    g_iconPathPrefix + "colour zoom plus scale 32x32.png";
static const std::string g_iconZoomMinus =
    g_iconPathPrefix + "colour zoom minus scale 32x32.png";
static const std::string g_iconViewFull =
    g_iconPathPrefix + "view-fullscreen.png";
static const std::string g_iconCutOn = g_iconPathPrefix + "cut on 32x32.png";
static const std::string g_iconCut = g_iconPathPrefix + "cut 32x32.png";
static const std::string g_iconGridOn = g_iconPathPrefix + "grid on 32x32.png";
static const std::string g_iconGrid = g_iconPathPrefix + "grid 32x32.png";
static const std::string g_iconRebinOn =
    g_iconPathPrefix + "rebin on 32x32.png";
static const std::string g_iconRebin = g_iconPathPrefix + "rebin 32x32.png";
static const std::string g_iconPeakListOn =
    g_iconPathPrefix + "Peak List on 32x32.png";
static const std::string g_iconPeakList =
    g_iconPathPrefix + "Peak List 32x32.png";

/** GUI for viewing a 2D slice out of a multi-dimensional workspace.
 * You can select which dimension to plot as X,Y, and the cut point
 * along the other dimension(s).
 *
 */
class EXPORT_OPT_MANTIDQT_SLICEVIEWER SliceViewer : public QWidget,
                                                    public ZoomablePeaksView {
  friend class SliceViewerWindow;

  Q_OBJECT

public:
  SliceViewer(QWidget *parent = nullptr);
  ~SliceViewer() override;

  void setWorkspace(const QString &wsName);
  void setWorkspace(Mantid::API::IMDWorkspace_sptr ws);
  Mantid::API::IMDWorkspace_sptr getWorkspace();
  void showControls(bool visible);
  void zoomBy(double factor);
  void loadColorMap(QString filename = QString());
  LineOverlay *getLineOverlay() { return m_lineOverlay; }
  Mantid::Kernel::VMD getSlicePoint() const { return m_slicePoint; }
  int getDimX() const;
  int getDimY() const;
  /// Methods for Python bindings
  QString getWorkspaceName() const;
  void setXYDim(int indexX, int indexY);
  void setXYDim(const QString &dimX, const QString &dimY);
  void setSlicePoint(int dim, double value);
  void setSlicePoint(const QString &dim, double value);
  double getSlicePoint(int dim) const;
  double getSlicePoint(const QString &dim) const;
  void setColorScaleMin(double min);
  void setColorScaleMax(double max);
  void setColorScaleLog(bool log);
  int getColorScaleType();
  void setColorScale(double min, double max, bool log);
  void setColorScale(double min, double max, int type);
  void setColorMapBackground(int r, int g, int b);
  double getColorScaleMin() const;
  double getColorScaleMax() const;
  bool getColorScaleLog() const;
  bool getFastRender() const;
  void setXYLimits(double xleft, double xright, double ybottom, double ytop);
  QwtDoubleInterval getXLimits() const;
  QwtDoubleInterval getYLimits() const;
  void setXYCenter(double x, double y);
  void openFromXML(const QString &xml);
  void toggleLineMode(bool /*lineMode*/);
  void setNormalization(Mantid::API::MDNormalization norm, bool update = true);
  Mantid::API::MDNormalization getNormalization() const;
  void setColorBarAutoScale(bool autoscale);

  /// Dynamic Rebinning-related Python bindings
  void setRebinThickness(int dim, double thickness);
  void setRebinNumBins(int xBins, int yBins);
  void setRebinMode(bool mode);
  void refreshRebin();

  /// Methods relating to peaks overlays.
  boost::shared_ptr<ProxyCompositePeaksPresenter> getPeaksPresenter() const;
  ProxyCompositePeaksPresenter *
  setPeaksWorkspaces(const QStringList &list); // For python binding
  void clearPeaksWorkspaces();                 // For python binding

  /* -- Methods from implementation of ZoomablePeaksView. --*/
  void zoomToRectangle(const PeakBoundingBox &box) override;
  void resetView() override;
  void detach() override;

  /* Methods associated with workspace observers. Driven by SliceViewerWindow */
  void peakWorkspaceChanged(
      const std::string &wsName,
      boost::shared_ptr<Mantid::API::IPeaksWorkspace> &changedPeaksWS);

  /// Load the state of the slice viewer from a Mantid project file
  void loadFromProject(const std::string &lines);
  /// Save the state of the slice viewer to a Mantid project file
  std::string saveToProject() const;
  /// Load the state of the dimension widgets from a Mantid project file
  void loadDimensionWidgets(const std::string &lines);
  /// Save the state of the dimension widgets to a Mantid project file
  std::string saveDimensionWidgets() const;

signals:
  /// Signal emitted when the X/Y index of the shown dimensions is changed
  void changedShownDim(size_t dimX, size_t dimY);
  /// Signal emitted when the slice point moves
  void changedSlicePoint(Mantid::Kernel::VMD slicePoint);
  /// Signal emitted when the LineViewer should be shown/hidden.
  void showLineViewer(bool /*_t1*/);
  /// Signal emitted when the PeaksViewer should be shown/hidden.
  void showPeaksViewer(bool /*_t1*/);
  /// Signal emitted when someone uses setWorkspace() on SliceViewer
  void workspaceChanged();
  /// Signal emitted when someone wants to see the options dialog
  void peaksTableColumnOptions();

public slots:
  void helpSliceViewer();
  void helpLineViewer();
  void helpPeaksViewer();
  void setFastRender(bool fast);
  void showInfoAt(double /*x*/, double /*y*/);
  // Change in view slots
  void checkForHKLDimension();
  void switchQWTRaster(bool useNonOrthogonal);
  void switchAxis();
  void changedShownDim(int index, int dim, int oldDim);
  void updateDisplaySlot(int index, double value);
  void resetZoom();
  void setXYLimitsDialog();
  void zoomInSlot();
  void zoomOutSlot();
  void zoomRectSlot(const QwtDoubleRect &rect);
  void panned(int /*unused*/, int /*unused*/);
  void magnifierRescaled(double /*unused*/);

  // Color scale slots
  void setColorScaleAutoFull();
  void setColorScaleAutoSlice();
  void colorRangeChanged();
  void loadColorMapSlot();
  void setTransparentZeros(bool transparent);
  void changeNormalizationNone();
  void changeNormalizationVolume();
  void changeNormalizationNumEvents();
  void onNormalizationChanged(const QString &normalizationKey);

  // Buttons or actions
  void clearLine();
  QPixmap getImage();
  void saveImage(const QString &filename = QString());
  void copyImageToClipboard();
  void onPeaksViewerOverlayOptions();
  // Non Orthogonal
  void setNonOrthogonalbtn();
  void disableOrthogonalAnalysisTools(bool checked);
  // Synced checkboxes
  void LineMode_toggled(bool /*checked*/);
  void SnapToGrid_toggled(bool /*checked*/);
  void RebinMode_toggled(bool /*checked*/);
  void autoRebin_toggled(bool /*checked*/);

  // Dynamic rebinning
  void rebinParamsChanged();
  void dynamicRebinComplete(bool error);
  // Peaks overlay
  void peakOverlay_clicked();
  // Aspect ratios
  void changeAspectRatioGuess();
  void changeAspectRatioAll();
  void changeAspectRatioUnlock();

protected:
  void dragEnterEvent(QDragEnterEvent *e) override;
  void dropEvent(QDropEvent *e) override;

private:
  enum AspectRatioType { Guess = 0, All = 1, Unlock = 2 };
  void loadSettings();
  void saveSettings();
  void setIconFromString(QAction *action, const std::string &iconName,
                         QIcon::Mode mode, QIcon::State state);
  void setIconFromString(QAbstractButton *btn, const std::string &iconName,
                         QIcon::Mode mode, QIcon::State state);
  void initMenus();
  void initZoomer();

  void updateDisplay(bool resetAxes = false);
  void updateDimensionSliceWidgets();
  void resetAxis(int axis,
                 const Mantid::Geometry::IMDDimension_const_sptr &dim);

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
  QString ensurePngExtension(const QString &fname) const;

  // Rescaler methods
  void updateAspectRatios();

  // Set aspect ratio type.
  void setAspectRatio(AspectRatioType type);

  /// Extracts and applies the color scaling for the current slice
  void applyColorScalingForCurrentSliceIfRequired();

  /// Apply the non orthogonal axis scale draw
  void applyNonOrthogonalAxisScaleDraw();

  /// Apply the orthogonal axis scale draw
  void applyOrthogonalAxisScaleDraw();

  /// Transfer data between QwtRasterDataMD
  void transferSettings(const API::QwtRasterDataMD *const from,
                        API::QwtRasterDataMD *to) const;

private:
  // -------------------------- Widgets ----------------------------

  /// Auto-generated UI controls.
  Ui::SliceViewerClass ui;

  /// Main plot object
  MantidQt::MantidWidgets::SafeQwtPlot *m_plot;

  /// Spectrogram plot
  QwtPlotSpectrogram *m_spect;

  /// Layout containing the spectrogram
  QHBoxLayout *m_spectLayout;

  /// Color bar indicating the color scale
  MantidQt::MantidWidgets::ColorBarWidget *m_colorBar;

  /// Vector of the widgets for slicing dimensions
  std::vector<DimensionSliceWidget *> m_dimWidgets;

  /// The LineOverlay widget for drawing line cross-sections (hidden at startup)
  LineOverlay *m_lineOverlay;

  /// The LineOverlay widget for drawing the outline of the rebinned workspace
  LineOverlay *m_overlayWSOutline;

  // PeakOverlay * m_peakOverlay;

  // NonOrthogonal Overlay for drawing axes
  NonOrthogonalOverlay *m_nonOrthogonalOverlay;

  /// Object for running algorithms in the background
  MantidQt::API::AlgorithmRunner *m_algoRunner;

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
  std::unique_ptr<API::QwtRasterDataMD> m_data;

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

  /// The calculated range of values ONLY in the currently viewed part of the
  /// slice
  QwtDoubleInterval m_colorRangeSlice;

  /// Use the log of the value for the color scale
  bool m_logColor;

  /// Menus
  QMenu *m_menuColorOptions, *m_menuView, *m_menuHelp, *m_menuLine, *m_menuFile,
      *m_menuPeaks;
  QAction *m_actionFileClose;
  QAction *m_actionTransparentZeros;
  QAction *m_actionNormalizeNone;
  QAction *m_actionNormalizeVolume;
  QAction *m_actionNormalizeNumEvents;
  QAction *m_actionRefreshRebin;
  QAction *m_lockAspectRatiosActionGuess;
  QAction *m_lockAspectRatiosActionAll;
  QAction *m_lockAspectRatiosActionUnlock;

  /// Synced menu/buttons
  MantidQt::API::SyncedCheckboxes *m_syncLineMode, *m_syncSnapToGrid,
      *m_syncRebinMode, *m_syncAutoRebin;

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
  boost::shared_ptr<MantidQt::API::MdSettings> m_mdSettings;

  /// Logger
  Mantid::Kernel::Logger m_logger;

  /// NonOrthogonal Fields
  std::unique_ptr<CoordinateTransform> m_coordinateTransform;
  bool m_firstNonOrthogonalWorkspaceOpen;
  bool m_nonOrthogonalDefault; // sets whether nonOrthogonalview should be shown
                               // as a default
  bool m_oldDimNonOrthogonal; // sets whether previous dimensions were displayed
  // as nonorthogonal, so if dims switch from orth -> nonOrth,
  // then nonOrth should default be shown again
  bool m_canSwitchScales; // stops qwtScaleDraw() from occuring in first set up

  // -------------------------- Controllers ------------------------
  boost::shared_ptr<CompositePeaksPresenter> m_peaksPresenter;

  boost::shared_ptr<ProxyCompositePeaksPresenter> m_proxyPeaksPresenter;

  /// Pointer to widget used for peaks sliding.
  DimensionSliceWidget *m_peaksSliderWidget;

  /// Object for choosing a PeakTransformFactory based on the workspace type.
  Mantid::Geometry::PeakTransformSelector m_peakTransformSelector;

  /// Plot rescaler. For fixed aspect ratios.
  QwtPlotRescaler *m_rescaler;

  static const QString NoNormalizationKey;
  static const QString VolumeNormalizationKey;
  static const QString NumEventsNormalizationKey;

  AspectRatioType m_aspectRatioType;
  AspectRatioType m_lastRatioState;
  QwtScaleDrawNonOrthogonal *m_nonOrthAxis0;
  QwtScaleDrawNonOrthogonal *m_nonOrthAxis1;

  bool m_holdDisplayUpdates;
};

} // namespace SliceViewer
} // namespace MantidQt

#endif // SLICEVIEWER_H
