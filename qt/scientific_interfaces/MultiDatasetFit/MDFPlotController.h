// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MDFPLOTCONTROLLER_H_
#define MDFPLOTCONTROLLER_H_

#include <QMap>
#include <QObject>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

// Forward declarations
class QwtPlot;
class QwtPlotZoomer;
class QwtPlotPanner;
class QwtPlotMagnifier;
class QTableWidget;
class QComboBox;
class QPushButton;
class QwtPlotCurve;

namespace Mantid {
namespace API {
class IFunction;
}
} // namespace Mantid

namespace MantidQt {
namespace MantidWidgets {
class RangeSelector;
}

namespace CustomInterfaces {

class MultiDatasetFit;

namespace MDF {

class DatasetPlotData;
class MDFFunctionPlotData;

/**
 * A class for controlling the plot widget and the displayed data.
 *
 * The instance keeps pointers to the plot and other control widgets but
 * not their positions within the parent window.
 *
 * Manages the plot tools.
 */
class PlotController : public QObject {
  Q_OBJECT
public:
  PlotController(MultiDatasetFit *parent, QwtPlot *plot, QTableWidget *table,
                 QComboBox *plotSelector, QPushButton *prev, QPushButton *next);
  ~PlotController() override;
  void clear(bool clearGuess = false);
  void update();
  int getCurrentIndex() const { return m_currentIndex; }
  bool isZoomEnabled() const;
  bool isPanEnabled() const;
  bool isRangeSelectorEnabled() const;
  void setGuessFunction(const QString &funStr);
  void updateGuessFunction(const Mantid::API::IFunction &fun);
signals:
  void currentIndexChanged(int /*_t1*/);
  void fittingRangeChanged(int /*_t1*/, double /*_t2*/, double /*_t3*/);
public slots:
  void enableZoom();
  void enablePan();
  void enableRange();
  void updateRange(int index);
  void showDataErrors(bool /*on*/);
  void resetRange();
  void zoomToRange();
  void exportCurrentPlot();
  void exportAllPlots();
  void showGuessFunction(bool ok);
private slots:
  void tableUpdated();
  void prevPlot();
  void nextPlot();
  void plotDataSet(int /*index*/);
  void updateFittingRange(double startX, double endX);
  void updateGuessPlot();

private:
  MultiDatasetFit *owner() const;
  void disableAllTools();
  template <class Tool> void enableTool(Tool *tool, int cursor);
  boost::shared_ptr<DatasetPlotData> getData(int i);
  void exportPlot(int index);
  QString makePyPlotSource(int index) const;
  void plotGuess();
  void hideGuess();

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
  MantidWidgets::RangeSelector *m_rangeSelector;
  ///@}

  /// The workspace table
  QTableWidget *m_table;
  QComboBox *m_plotSelector;
  QPushButton *m_prevPlot;
  QPushButton *m_nextPlot;
  QMap<int, boost::shared_ptr<DatasetPlotData>> m_plotData;
  int m_currentIndex;
  bool m_showDataErrors;

  /// Function guess
  boost::scoped_ptr<MDFFunctionPlotData> m_guessFunctionData;
  bool m_showGuessFunction;
};

} // namespace MDF
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /*MDFPLOTCONTROLLER_H_*/
