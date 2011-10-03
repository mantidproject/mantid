#ifndef SLICEVIEWER_H
#define SLICEVIEWER_H

#include <QtGui/QWidget>
#include "ui_SliceViewer.h"
#include <QtGui/qdialog.h>
#include <qwt_plot_spectrogram.h>
#include <qwt_plot.h>
#include <qwt_raster_data.h>

class SliceViewer : public QWidget
{
  Q_OBJECT

public:
  SliceViewer(QWidget *parent = 0);
  ~SliceViewer();

private:
  void initLayout();

private:
  /// Auto-generated UI controls.
  Ui::SliceViewerClass ui;

  /// Main plot object
  QwtPlot * m_plot;

  /// Spectrogram plot
  QwtPlotSpectrogram * m_spect;

  /// Layout containing the spectrogram
  QVBoxLayout * m_spectLayout;

  /// Data to be shown
  QwtRasterData * m_data;


};

#endif // SLICEVIEWER_H
