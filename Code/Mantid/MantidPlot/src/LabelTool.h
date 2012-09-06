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

#include <QList>
#include <QMenu>
#include <QSet>

#include "QwtErrorPlotCurve.h"
#include <qwt_plot_marker.h>
#include <qwt_picker.h>
#include <qwt_scale_widget.h>
#include <qpoint.h>
#include <qpolygon.h>
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>

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
 LabelTool(Graph *graph, const QObject *status_target=NULL, const char *status_slot="");
 virtual ~LabelTool();
 virtual int rtti() const {return PlotToolInterface::Rtti_LabelTool;};
 
 void removeTextBox();

protected:

private:
  QwtPicker *m_canvasPicker;
  QwtPicker *m_xAxisPicker;
  QwtPicker *m_yAxisPicker;
  
  QSet<QString> workspaceNames();
  QSet<QString> logValues();
  QList<MantidMatrixCurve *> m_mantidMatrixCurves;

  void populateMantidCurves();
  void blankCanvasClick();
  void dataPointClicked();  

  /// Relating to the point where the canvas is clicked.
  double m_xPos;
  double m_yPos;
  std::string m_axisX;
  std::string m_axisY;
  std::string m_axisCoordsX;
  std::string m_axisCoordsY;
  std::string m_error;
  std::string m_dataCoords;

private slots:

  void canvasClicked(const QwtPolygon &);
  void xAxisClicked(const QwtPolygon &);
  void yAxisClicked(const QwtPolygon &);

  void insertTextBox();
};


#endif // ifndef LABEL_TOOL_H