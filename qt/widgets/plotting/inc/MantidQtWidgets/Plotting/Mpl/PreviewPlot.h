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
#include "MantidQtWidgets/Plotting/AxisID.h"
#include "MantidQtWidgets/Plotting/DllOption.h"

#include <Poco/NObserver.h>

#include <QWidget>
#include <list>

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

  void addSpectrum(const QString &curveName,
                   const Mantid::API::MatrixWorkspace_sptr &ws,
                   const size_t wsIndex = 0,
                   const QColor &curveColour = QColor());
  void addSpectrum(const QString &curveName, const QString &wsName,
                   const size_t wsIndex = 0,
                   const QColor &curveColour = QColor());
  void removeSpectrum(const QString &curveName);
  void setAxisRange(const QPair<double, double> &range,
                    AxisID axisID = AxisID::XBottom);

public slots:
  void clear();
  void resizeX();

private:
  void onWorkspaceRemoved(Mantid::API::WorkspacePreDeleteNotification_ptr nf);
  void
  onWorkspaceReplaced(Mantid::API::WorkspaceBeforeReplaceNotification_ptr nf);
  Widgets::MplCpp::Line2D createLine(Widgets::MplCpp::Axes &axes,
                                     const Mantid::API::MatrixWorkspace &ws,
                                     const size_t wsIndex, const QColor &lineColor);
  void removeLines(const Mantid::API::MatrixWorkspace &ws);
  void replaceLines(const Mantid::API::MatrixWorkspace &oldWS,
                    const Mantid::API::MatrixWorkspace &newWS);

  struct Line2DInfo {
    Widgets::MplCpp::Line2D line;
    QString name;
    // Non-owning pointer to the source workspace
    // It is only safe to compare this with another workspace pointer
    const Mantid::API::MatrixWorkspace * workspace;
    const size_t wsIndex;
    const QColor curveColour;
  };

  Widgets::MplCpp::FigureCanvasQt *m_canvas;
  std::list<Line2DInfo> m_lines;
  // Poco Observers for ADS Notifications
  Poco::NObserver<PreviewPlot, Mantid::API::WorkspacePreDeleteNotification>
      m_wsRemovedObserver;
  Poco::NObserver<PreviewPlot, Mantid::API::WorkspaceBeforeReplaceNotification>
      m_wsReplacedObserver;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDQT_PLOTTING_MPL_PREVIEWPLOT_H_
