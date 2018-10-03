#ifndef LABEL_TOOL_H
#define LABEL_TOOL_H

#include "PlotToolInterface.h"
#include <QObject>
#include <QSet>
#include <QString>
#include <QVector>
#include <qwt_polygon.h>

class MantidMatrixCurve;

class QAction;
class QGraph;
class QMenu;
class QPoint;

class QwtPicker;
class QwtSymbol;

/// Labelling tool for graphs, which can also read points on a graph, as well as
/// the axis.
class LabelTool : public QObject,
                  public QVector<QPoint>,
                  public PlotToolInterface {
  Q_OBJECT
public:
  explicit LabelTool(Graph *graph);
  ~LabelTool() override;
  int rtti() const override { return PlotToolInterface::Rtti_LabelTool; }

  void removeTextBox();

protected:
private:
  QwtPicker *m_canvasPicker;
  QwtPicker *m_xAxisPicker;
  QwtPicker *m_yAxisPicker;

  QSet<QString> workspaceNames();
  // Future idea to display in the drop-down menu: QString workspaceTitle();
  // Future idea to display in the drop-down menu: QString
  // workspaceInstrument();
  QSet<QString> logValues();

  void populateMantidCurves();
  void blankRegionClicked();
  void dataPointClicked();

  /// Member variables relating to the point where the canvas is clicked.
  double m_xPos;
  double m_yPos;
  QString m_xPosSigFigs;
  QString m_yPosSigFigs;
  QString m_error;
  QString m_dataCoords;
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
  void showLogValuesDialog();
};

#endif // ifndef LABEL_TOOL_H
