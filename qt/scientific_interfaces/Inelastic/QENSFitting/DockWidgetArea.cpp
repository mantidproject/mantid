// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "DockWidgetArea.h"
#include "FitDataView.h"
#include "FitPlotView.h"

namespace MantidQt::CustomInterfaces::Inelastic {

DockWidgetArea::DockWidgetArea(QWidget *parent) : QMainWindow(parent) {
  QMainWindow::setWindowFlags(Qt::Widget);
  setDockOptions(QMainWindow::AnimatedDocks);

  m_fitPropertyBrowser = new InelasticFitPropertyBrowser(this);
  m_fitPropertyBrowser->setFeatures(QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);

  QDockWidget *plotViewArea = new QDockWidget(this);
  plotViewArea->setWindowTitle("Mini plots");
  m_fitPlotView = new FitPlotView(this);
  plotViewArea->setWidget(m_fitPlotView);
  plotViewArea->setFeatures(QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);

  QFrame *line = new QFrame(this);
  line->setFrameShape(QFrame::HLine);
  line->setFrameShadow(QFrame::Raised);
  line->setLineWidth(0);
  line->setMidLineWidth(1);
  setCentralWidget(line);

  addDockWidget(Qt::BottomDockWidgetArea, m_fitPropertyBrowser);
  addDockWidget(Qt::BottomDockWidgetArea, plotViewArea);
  resizeDocks({m_fitPropertyBrowser, plotViewArea}, {20, 20}, Qt::Horizontal);
}

void DockWidgetArea::setFitDataView(FitDataView *fitDataView) {
  QDockWidget *dataViewArea = new QDockWidget(this);
  dataViewArea->setWindowTitle("Data Input");
  m_fitDataView = fitDataView;
  dataViewArea->setWidget(m_fitDataView);
  dataViewArea->setFeatures(QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
  addDockWidget(Qt::TopDockWidgetArea, dataViewArea);
}

} // namespace MantidQt::CustomInterfaces::Inelastic
