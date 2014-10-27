#ifndef LABEL_TOOL_H
#define LABEL_TOOL_H

#include "ApplicationWindow.h"
#include "Graph.h"
#include "LegendWidget.h"
#include "TextDialog.h"

#include "Plot.h"
#include "PlotCurve.h"
#include "PlotToolInterface.h"

#include "Mantid/MantidMatrixCurve.h" 
#include "MantidAPI/AnalysisDataService.h"
#include "MantidQtAPI/MantidQwtWorkspaceData.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/Workspace.h"

#include <QList>
#include <QMenu>
#include <QSet>

#include "QwtErrorPlotCurve.h"
#include <qwt_scale_widget.h>
#include <qpoint.h>
#include <qpolygon.h>
#include <qwt_picker.h>
#include <qwt_plot.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>
#include <qwt_text_label.h>
#include <qwt_symbol.h>
#include <qwt_text.h>

#include <iomanip>

class ApplicationWindow;
class QwtPlotCurve;
class QPoint;
class QwtPlot;
class QAction;
class QMenu;

/// Labelling tool for graphs, which can also read points on a graph, as well as the axis.
class LabelTool : public QObject, public QVector<QPoint>, public PlotToolInterface
{
  Q_OBJECT
public: 
 LabelTool(Graph *graph);
 virtual ~LabelTool();
 virtual int rtti() const {return PlotToolInterface::Rtti_LabelTool;};
 
 void removeTextBox();

protected:

private:
  QwtPicker *m_canvasPicker;
  QwtPicker *m_xAxisPicker;
  QwtPicker *m_yAxisPicker;
  
  QSet<QString> workspaceNames();
  // Future idea to display in the drop-down menu: QString workspaceTitle();
  // Future idea to display in the drop-down menu: QString workspaceInstrument();
  QSet<QString> logValues();
  
  void populateMantidCurves();
  void blankRegionClicked();
  void dataPointClicked(); 

  /// Member variables relating to the point where the canvas is clicked.
  double m_xPos;
  double m_yPos;
  std::string m_axisCoordsX;
  std::string m_axisCoordsY;
  std::string m_xPosSigFigs;
  std::string m_yPosSigFigs;
  std::string m_error;
  std::string m_dataCoords;
  QString m_curveWsName;

  /// List of curves of type MantidCurve.
  QList<MantidMatrixCurve *> m_mantidMatrixCurves;

  /// List of symbols on the plot.
  QList<QwtSymbol *> m_symbols;

private slots:

  void graphAreaClicked(const QwtPolygon &);
  void xAxisClicked(const QwtPolygon &);
  void yAxisClicked(const QwtPolygon &);

	void insertLegend();
	void insertTextBox();
  void insertXCoord();
  void insertYCoord();
  void insertDataCoord();
  void insertErrorValue();
};


#endif // ifndef LABEL_TOOL_H
