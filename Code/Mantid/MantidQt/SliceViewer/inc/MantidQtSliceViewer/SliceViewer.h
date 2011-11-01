#ifndef SLICEVIEWER_H
#define SLICEVIEWER_H

#include "DimensionSliceWidget.h"
#include "DllOption.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "ColorBarWidget.h"
#include "QwtRasterDataMD.h"
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
#include "MantidQtAPI/MantidColorMap.h"


class EXPORT_OPT_MANTIDQT_SLICEVIEWER SliceViewer : public QWidget
{
  Q_OBJECT

public:
  SliceViewer(QWidget *parent = 0);
  ~SliceViewer();

  void setWorkspace(Mantid::API::IMDWorkspace_sptr ws);
  void showControls(bool visible);
  void zoomBy(double factor);

public slots:
  void changedShownDim(int index, int dim, int oldDim);
  void updateDisplaySlot(int index, double value);
  void resetZoom();
  void showInfoAt(double, double);
  void colorRangeFullSlot();
  void colorRangeSliceSlot();
  void colorRangeChanged();
  void zoomInSlot();
  void zoomOutSlot();


private:
  void loadSettings();
  void saveSettings();
  void initMenus();
  void initZoomer();

  void updateDisplay(bool resetAxes = false);
  void updateDimensionSliceWidgets();
  void resetAxis(int axis, Mantid::Geometry::IMDDimension_const_sptr dim);

  void findRangeFull();
  void findRangeSlice();


private:
  /// Auto-generated UI controls.
  Ui::SliceViewerClass ui;

  /// Set to true once the first workspace has been loaded in it
  bool m_firstWorkspaceOpen;

  /// Main plot object
  QwtPlot * m_plot;

  /// Spectrogram plot
  QwtPlotSpectrogram * m_spect;

  /// Layout containing the spectrogram
  QHBoxLayout * m_spectLayout;

  /// Color map in use
  MantidColorMap * m_colorMap;

  /// File of the last loaded color map.
  QString m_currentColorMapFile;

  /// Color bar indicating the color scale
  ColorBarWidget * m_colorBar;

  /// Vector of the widgets for slicing dimensions
  std::vector<DimensionSliceWidget *> m_dimWidgets;

  /// Vector of the dimensions to show.
  std::vector<Mantid::Geometry::MDHistoDimension_sptr> m_dimensions;

  /// Data presenter
  QwtRasterDataMD * m_data;

  /// Workspace being shown
  Mantid::API::IMDWorkspace_sptr m_ws;

  /// The X and Y dimensions being plotted
  Mantid::Geometry::IMDDimension_const_sptr m_X;
  Mantid::Geometry::IMDDimension_const_sptr m_Y;
  size_t m_dimX;
  size_t m_dimY;

  /// The range of values to fit in the color map.
  QwtDoubleInterval m_colorRange;

  /// The calculated range of values in the FULL data set
  QwtDoubleInterval m_colorRangeFull;

  /// The calculated range of values ONLY in the currently viewed part of the slice
  QwtDoubleInterval m_colorRangeSlice;

  /// Use the log of the value for the color scale
  bool m_logColor;

  /// Menus
  QMenu * m_menuColorOptions;
  QMenu * m_menuView;

};

#endif // SLICEVIEWER_H
