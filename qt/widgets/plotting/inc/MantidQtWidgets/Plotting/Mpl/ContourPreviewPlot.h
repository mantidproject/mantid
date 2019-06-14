// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_PLOTTING_MPL_CONTOURPREVIEWPLOT_H_
#define MANTIDQT_PLOTTING_MPL_CONTOURPREVIEWPLOT_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Plotting/DllOption.h"

#include <Poco/NObserver.h>

#include <QColor>
#include <QWidget>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

class Axes;
class FigureCanvasQt;

} // namespace MplCpp
} // namespace Widgets

namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_PLOTTING ContourPreviewPlot : public QWidget {
  Q_OBJECT

public:
  ContourPreviewPlot(QWidget *parent = nullptr, bool observeADS = true);
  ~ContourPreviewPlot() override;

  void watchADS(bool on);

  void setCanvasColour(QColor const &colour);

  void setWorkspace(Mantid::API::MatrixWorkspace_sptr workspace);

private:
  void createLayout();

  void onWorkspaceRemoved(Mantid::API::WorkspacePreDeleteNotification_ptr nf);
  void
  onWorkspaceReplaced(Mantid::API::WorkspaceBeforeReplaceNotification_ptr nf);

  /// Canvas objects
  Widgets::MplCpp::FigureCanvasQt *m_canvas;

  /// Observers for ADS Notifications
  Poco::NObserver<ContourPreviewPlot,
                  Mantid::API::WorkspacePreDeleteNotification>
      m_wsRemovedObserver;
  Poco::NObserver<ContourPreviewPlot,
                  Mantid::API::WorkspaceBeforeReplaceNotification>
      m_wsReplacedObserver;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTIDQT_PLOTTING_MPL_CONTOURPREVIEWPLOT_H_ */
