#ifndef MDFPLOTCONTROLLER_H_
#define MDFPLOTCONTROLLER_H_

#include <QObject>
#include <QMap>
#include <boost/shared_ptr.hpp>

// Forward declarations
class QwtPlot;
class QwtPlotZoomer;
class QwtPlotPanner;
class QwtPlotMagnifier;
class QTableWidget;
class QComboBox;
class QPushButton;

namespace MantidQt
{
namespace MantidWidgets
{
class RangeSelector;
}

namespace CustomInterfaces
{

class MultiDatasetFit;

namespace MDF
{

class DatasetPlotData;

/**
  * A class for controlling the plot widget and the displayed data.
  *
  * The instance keeps pointers to the plot and other control widgets but
  * not their positions within the parent window.
  *
  * Manages the plot tools.
  */
class PlotController: public QObject
{
  Q_OBJECT
public:
  PlotController(MultiDatasetFit *parent, QwtPlot *plot, QTableWidget *table, QComboBox *plotSelector, QPushButton *prev, QPushButton *next);
  ~PlotController();
  void clear();
  void update();
  int getCurrentIndex() const {return m_currentIndex;}
  bool isZoomEnabled() const;
  bool isPanEnabled() const;
  bool isRangeSelectorEnabled() const;
signals:
  void currentIndexChanged(int);
  void fittingRangeChanged(int, double, double);
public slots:
  void enableZoom();
  void enablePan();
  void enableRange();
  void updateRange(int index);
private slots:
  void tableUpdated();
  void prevPlot();
  void nextPlot();
  void plotDataSet(int);
  void updateFittingRange(double startX, double endX);
private:
  MultiDatasetFit *owner() const;
  void disableAllTools();
  template<class Tool>
  void enableTool(Tool* tool, int cursor);
  bool eventFilter(QObject *widget, QEvent *evn);
  void resetRange();
  void zoomToRange();
  boost::shared_ptr<DatasetPlotData> getData(int i);

  /// The plot widget
  QwtPlot *m_plot;

  ///@name Plot tools
  ///@{
  /// The zoomer
  QwtPlotZoomer *m_zoomer;
  /// The panner
  QwtPlotPanner *m_panner;
  /// The magnifier
  QwtPlotMagnifier *m_magnifier;
  /// The fitting range selector
  MantidWidgets::RangeSelector* m_rangeSelector;
  ///@}

  /// The workspace table
  QTableWidget *m_table;
  QComboBox *m_plotSelector;
  QPushButton *m_prevPlot;
  QPushButton *m_nextPlot;
  QMap<int,boost::shared_ptr<DatasetPlotData>> m_plotData;
  int m_currentIndex;
};

} // MDF
} // CustomInterfaces
} // MantidQt


#endif /*MDFPLOTCONTROLLER_H_*/
