// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_PLOTTING_MPL_PREVIEWPLOT_H_
#define MANTIDQT_PLOTTING_MPL_PREVIEWPLOT_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/MplCpp/Line2D.h"
<<<<<<< HEAD
#include "MantidQtWidgets/MplCpp/ZoomTool.h"
||||||| parent of d0c18a6... Merge ZoomTool and PanTool
#include "MantidQtWidgets/MplCpp/PanTool.h"
#include "MantidQtWidgets/MplCpp/ZoomTool.h"
=======
#include "MantidQtWidgets/MplCpp/PanZoomTool.h"
>>>>>>> d0c18a6... Merge ZoomTool and PanTool
#include "MantidQtWidgets/Plotting/AxisID.h"
#include "MantidQtWidgets/Plotting/DllOption.h"

#include <Poco/NObserver.h>

#include <QWidget>
#include <list>

class QAction;
class QActionGroup;

namespace MantidQt {
namespace Widgets {
namespace MplCpp {
class Axes;
class FigureCanvasQt;
} // namespace MplCpp
} // namespace Widgets
namespace MantidWidgets {

/**
 * Displays several workpaces on a matplotlib figure
 */
class EXPORT_OPT_MANTIDQT_PLOTTING PreviewPlot : public QWidget {
  Q_OBJECT

public:
  PreviewPlot(QWidget *parent = nullptr, bool watchADS = true);
  ~PreviewPlot();

  void addSpectrum(const QString &lineLabel,
                   const Mantid::API::MatrixWorkspace_sptr &ws,
                   const size_t wsIndex = 0,
                   const QColor &lineColour = QColor());
  void addSpectrum(const QString &lineName, const QString &wsName,
                   const size_t wsIndex = 0,
                   const QColor &lineColour = QColor());
  void removeSpectrum(const QString &lineName);
  void setAxisRange(const QPair<double, double> &range,
                    AxisID axisID = AxisID::XBottom);

public slots:
  void clear();
  void resizeX();
  void resetView();
  void showLegend(const bool visible);

protected:
  bool eventFilter(QObject *watched, QEvent *evt) override;

private:
  bool handleMousePressEvent(QMouseEvent *evt);
  bool handleMouseReleaseEvent(QMouseEvent *evt);
  void showContextMenu(QMouseEvent *evt);

  struct Line2DInfo {
    Widgets::MplCpp::Line2D line;
    // Non-owning pointer to the source workspace
    // It is only safe to compare this with another workspace pointer
    const Mantid::API::MatrixWorkspace *workspace;
    const size_t wsIndex;
    const QString label;
    const QColor colour;
  };

  void createLayout();
  void createActions();

  bool legendIsVisible() const;

  void onWorkspaceRemoved(Mantid::API::WorkspacePreDeleteNotification_ptr nf);
  void
  onWorkspaceReplaced(Mantid::API::WorkspaceBeforeReplaceNotification_ptr nf);

  Line2DInfo createLineInfo(Widgets::MplCpp::Axes &axes,
                            const Mantid::API::MatrixWorkspace &ws,
                            const size_t wsIndex, const QString &curveName,
                            const QColor &lineColour);
  void removeLines(const Mantid::API::MatrixWorkspace &ws);
  void replaceLineData(const Mantid::API::MatrixWorkspace &oldWS,
                       const Mantid::API::MatrixWorkspace &newWS);
  void regenerateLegend();
  void removeLegend();

  void switchPlotTool(QAction *selected);
  void setXScaleType(QAction *selected);
  void setYScaleType(QAction *selected);
  void setScaleType(AxisID id, QString actionName);
  void toggleLegend(const bool checked);

  // Canvas objects
  Widgets::MplCpp::FigureCanvasQt *m_canvas;
  std::list<Line2DInfo> m_lines;

  // Canvas tools
  Widgets::MplCpp::PanZoomTool m_panZoomTool;

  // Observers for ADS Notifications
  Poco::NObserver<PreviewPlot, Mantid::API::WorkspacePreDeleteNotification>
      m_wsRemovedObserver;
  Poco::NObserver<PreviewPlot, Mantid::API::WorkspaceBeforeReplaceNotification>
      m_wsReplacedObserver;

  // Context menu actions
  QActionGroup *m_contextPlotTools;
  QAction *m_contextResetView;
  QActionGroup *m_contextXScale, *m_contextYScale;
  QAction *m_contextLegend;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDQT_PLOTTING_MPL_PREVIEWPLOT_H_
