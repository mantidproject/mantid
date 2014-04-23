#include "MantidQtCustomInterfaces/Muon/ALCDataLoadingView.h"

#include <QMessageBox>

#include <qwt_symbol.h>

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
    connect(m_ui.load, SIGNAL(clicked()), this, SIGNAL(loadRequested()));

    m_ui.dataPlot->setCanvasBackground(Qt::white);
    m_ui.dataPlot->setAxisFont(QwtPlot::xBottom, m_widget->font());
    m_ui.dataPlot->setAxisFont(QwtPlot::yLeft, m_widget->font());

    m_dataCurve->setStyle(QwtPlotCurve::NoCurve);
    m_dataCurve->setSymbol(QwtSymbol(QwtSymbol::Ellipse, QBrush(), QPen(), QSize(7,7)));
    m_dataCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
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

  void ALCDataLoadingView::setDataCurve(const QwtData& data)
  {
    m_dataCurve->setData(data);
    m_ui.dataPlot->replot();
  }

  void ALCDataLoadingView::displayError(const std::string& error)
  {
    QMessageBox::critical(m_widget, "Loading error", QString::fromStdString(error));
  }

} // namespace CustomInterfaces
} // namespace MantidQt
