#include "MantidQtCustomInterfaces/Muon/ALCDataLoadingView.h"

#include <QMessageBox>

namespace MantidQt
{
namespace CustomInterfaces
{
  ALCDataLoadingView::ALCDataLoadingView(QWidget* widget)
    : m_widget(widget), m_dataCurve(new QwtPlotCurve())
  {}

  void ALCDataLoadingView::initialize()
  {
    m_ui.setupUi(m_widget);
    connect(m_ui.load, SIGNAL(pressed()), this, SIGNAL(loadData()));

    m_ui.dataPlot->setCanvasBackground(Qt::white);
    m_ui.dataPlot->setAxisFont(QwtPlot::xBottom, m_widget->font());
    m_ui.dataPlot->setAxisFont(QwtPlot::yLeft, m_widget->font());

    m_dataCurve->attach(m_ui.dataPlot);
  }

  std::string ALCDataLoadingView::firstRun() const
  {
    return m_ui.firstRun->getFirstFilename().toStdString();
  }

  std::string ALCDataLoadingView::lastRun() const
  {
    return m_ui.lastRun->getFirstFilename().toStdString();
  }

  std::string ALCDataLoadingView::log() const
  {
    return m_ui.log->text().toStdString();
  }

  void ALCDataLoadingView::displayData(MatrixWorkspace_const_sptr data)
  {
    const Mantid::MantidVec& dataX = data->readX(0);
    const Mantid::MantidVec& dataY = data->readY(0);

    m_dataCurve->setData(&dataX[0], &dataY[0], static_cast<int>(data->blocksize()));

    m_ui.dataPlot->replot();
  }

  void ALCDataLoadingView::displayError(const std::string& error)
  {
    QMessageBox::critical(m_widget, "Loading error", QString::fromStdString(error));
  }

} // namespace CustomInterfaces
} // namespace MantidQt
