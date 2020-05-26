// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectDockWidgetArea.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectDockWidgetArea::IndirectDockWidgetArea(QWidget *parent)
    : QMainWindow(parent), m_uiForm(new Ui::IndirectDockWidgetArea) {
  m_uiForm->setupUi(this);
  QMainWindow::setWindowFlags(Qt::Widget);
  m_fitPropertyBrowser = new IndirectFitPropertyBrowser();
  QDockWidget *plotViewArea = new QDockWidget();
  m_fitPlotView = new IndirectFitPlotView();
  plotViewArea->setWidget(m_fitPlotView);
  plotViewArea->setFeatures(QDockWidget::DockWidgetFloatable);
  addDockWidget(Qt::BottomDockWidgetArea, m_fitPropertyBrowser);
  addDockWidget(Qt::BottomDockWidgetArea, plotViewArea);
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
  resizeDocks({m_fitPropertyBrowser, plotViewArea}, {20, 20}, Qt::Horizontal);
#endif
  m_fitDataView = m_uiForm->fitDataView;
}
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt