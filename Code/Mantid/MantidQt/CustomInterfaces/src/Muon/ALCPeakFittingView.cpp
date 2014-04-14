#include "MantidQtCustomInterfaces/Muon/ALCPeakFittingView.h"

#include "MantidAPI/FunctionFactory.h"

#include <qwt_symbol.h>

namespace MantidQt
{
namespace CustomInterfaces
{

ALCPeakFittingView::ALCPeakFittingView(QWidget* widget)
  : m_widget(widget), m_ui(), m_dataCurve(new QwtPlotCurve()), m_fittedCurve(new QwtPlotCurve())
{}

std::string ALCPeakFittingView::function() const
{
  return m_ui.peaks->getFunctionString().toStdString();
}

void ALCPeakFittingView::initialize()
{
  m_ui.setupUi(m_widget);

  connect(m_ui.fit, SIGNAL(clicked()), this, SIGNAL(fitRequested()));

  m_dataCurve->setStyle(QwtPlotCurve::NoCurve);
  m_dataCurve->setSymbol(QwtSymbol(QwtSymbol::Ellipse, QBrush(), QPen(), QSize(7,7)));
  m_dataCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
  m_dataCurve->attach(m_ui.plot);

  m_fittedCurve->setPen(QPen(Qt::red, 1.5));
  m_fittedCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
  m_fittedCurve->attach(m_ui.plot);
}

void ALCPeakFittingView::setDataCurve(const QwtData& data)
{
  m_dataCurve->setData(data);
  m_ui.plot->replot();
}

void ALCPeakFittingView::setFittedCurve(const QwtData& data)
{
  m_fittedCurve->setData(data);
  m_ui.plot->replot();
}

void ALCPeakFittingView::setFunction(const std::string& newFunction)
{
  m_ui.peaks->setFunction(QString::fromStdString(newFunction));
}

} // namespace CustomInterfaces
} // namespace Mantid

