#include "MantidQtCustomInterfaces/Muon/ALCDataLoadingView.h"

#include <QMessageBox>
#include <qwt_plot_curve.h>

namespace MantidQt
{
namespace CustomInterfaces
{
  ALCDataLoadingView::ALCDataLoadingView(QWidget* widget)
    : m_presenter(this), m_widget(widget)
  {}

  void ALCDataLoadingView::initialize()
  {
    m_ui.setupUi(m_widget);
    connect(m_ui.load, SIGNAL(pressed()), this, SIGNAL(loadData()));

    m_ui.dataPlot->setCanvasBackground(Qt::white);
    m_ui.dataPlot->setAxisFont(QwtPlot::xBottom, m_widget->font());
    m_ui.dataPlot->setAxisFont(QwtPlot::yLeft, m_widget->font());

    m_presenter.initialize();
  }

  std::string ALCDataLoadingView::firstRun()
  {
    return m_ui.firstRun->getFirstFilename().toStdString();
  }

  std::string ALCDataLoadingView::lastRun()
  {
    return m_ui.lastRun->getFirstFilename().toStdString();
  }

  std::string ALCDataLoadingView::log()
  {
    return m_ui.log->text().toStdString();
  }

  void ALCDataLoadingView::displayData(MatrixWorkspace_const_sptr data)
  {
    using Mantid::MantidVec;
    const MantidVec& dataX = data->readX(0);
    const MantidVec& dataY = data->readY(0);

    QwtPlotCurve* curve = new QwtPlotCurve();
    curve->setData(&dataX[0], &dataY[0], static_cast<int>(data->blocksize()));
    curve->attach(m_ui.dataPlot);

    m_ui.dataPlot->replot();
  }

  void ALCDataLoadingView::displayError(const std::string& error)
  {
    QMessageBox::critical(m_widget, "Loading error", QString::fromStdString(error));
  }

} // namespace CustomInterfaces
} // namespace MantidQt
