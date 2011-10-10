#ifndef SLICEVIEWER_H
#define SLICEVIEWER_H

#include "DimensionSliceWidget.h"
#include "MantidAPI/IMDWorkspace.h"
#include "QwtRasterDataMD.h"
#include "ui_SliceViewer.h"
#include <QtGui/qdialog.h>
#include <QtGui/QWidget>
#include <qwt_plot_spectrogram.h>
#include <qwt_plot.h>
#include <qwt_raster_data.h>
#include <vector>

class SliceViewer : public QWidget
{
  Q_OBJECT

public:
  SliceViewer(QWidget *parent = 0);
  ~SliceViewer();

  void setWorkspace(Mantid::API::IMDWorkspace_sptr ws);


public slots:
  void changedShownDim(int index, int dim);
  void updateDisplaySlot(int index, double value);
  void updateDisplay();

private:
  void initLayout();
  void updateDimensionSliceWidgets();

private:
  /// Auto-generated UI controls.
  Ui::SliceViewerClass ui;

  /// Main plot object
  QwtPlot * m_plot;

  /// Spectrogram plot
  QwtPlotSpectrogram * m_spect;

  /// Layout containing the spectrogram
  QVBoxLayout * m_spectLayout;

  /// Vector of the widgets for slicing dimensions
  std::vector<DimensionSliceWidget *> m_dimWidgets;


  /// Data presenter
  QwtRasterDataMD * m_data;

  /// Workspace being shown
  Mantid::API::IMDWorkspace_sptr m_ws;

  /// The X and Y dimensions being plotted
  Mantid::Geometry::IMDDimension_const_sptr m_X;
  Mantid::Geometry::IMDDimension_const_sptr m_Y;

};

#endif // SLICEVIEWER_H
