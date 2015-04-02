#include "MantidQtCustomInterfaces/Muon/ALCPeakFittingView.h"

#include "MantidQtAPI/HelpWindow.h"

#include <qwt_symbol.h>

namespace MantidQt
{
namespace CustomInterfaces
{

ALCPeakFittingView::ALCPeakFittingView(QWidget* widget)
  : m_widget(widget), m_ui(), m_dataCurve(new QwtPlotCurve()), m_fittedCurve(new QwtPlotCurve()),
    m_peakPicker(NULL)
{}

IFunction_const_sptr ALCPeakFittingView::function(QString index) const
{
  return m_ui.peaks->getFunctionByIndex(index);
}

boost::optional<QString> ALCPeakFittingView::currentFunctionIndex() const
{
  return m_ui.peaks->currentFunctionIndex();
}

IPeakFunction_const_sptr ALCPeakFittingView::peakPicker() const
{
  return m_peakPicker->peak();
}

void ALCPeakFittingView::initialize()
{
  m_ui.setupUi(m_widget);

  connect(m_ui.fit, SIGNAL(clicked()), this, SIGNAL(fitRequested()));

  m_ui.plot->setCanvasBackground(Qt::white);
  m_ui.plot->setAxisFont(QwtPlot::xBottom, m_widget->font());
  m_ui.plot->setAxisFont(QwtPlot::yLeft, m_widget->font());

  m_dataCurve->setStyle(QwtPlotCurve::NoCurve);
  m_dataCurve->setSymbol(QwtSymbol(QwtSymbol::Ellipse, QBrush(), QPen(), QSize(7,7)));
  m_dataCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
  m_dataCurve->attach(m_ui.plot);

  m_fittedCurve->setPen(QPen(Qt::red, 1.5));
  m_fittedCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
  m_fittedCurve->attach(m_ui.plot);

  // XXX: Being a QwtPlotItem, should get deleted when m_ui.plot gets deleted (auto-delete option)
  m_peakPicker = new MantidWidgets::PeakPicker(m_ui.plot, Qt::red);

  connect(m_peakPicker, SIGNAL(changed()), SIGNAL(peakPickerChanged()));

  connect(m_ui.peaks, SIGNAL(currentFunctionChanged()), SIGNAL(currentFunctionChanged()));
  connect(m_ui.peaks, SIGNAL(parameterChanged(QString,QString)),
          SIGNAL(parameterChanged(QString,QString)));

  connect(m_ui.help, SIGNAL(clicked()), this, SLOT(help()));
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

void ALCPeakFittingView::setFunction(const IFunction_const_sptr& newFunction)
{
  if (newFunction)
  {
    size_t nParams = newFunction->nParams();
    for (size_t i=0; i<nParams; i++) {

      QString name = QString::fromStdString(newFunction->parameterName(i));
      double value = newFunction->getParameter(i);
      double error = newFunction->getError(i);

      m_ui.peaks->setParameter(name,value);
      m_ui.peaks->setParamError(name,error);
    }
  }
  else
  {
    m_ui.peaks->clear();
  }
}

void ALCPeakFittingView::setParameter(const QString& funcIndex, const QString& paramName, double value)
{
  m_ui.peaks->setParameter(funcIndex, paramName, value);
}

void ALCPeakFittingView::setPeakPickerEnabled(bool enabled)
{
  m_peakPicker->setEnabled(enabled);
  m_peakPicker->setVisible(enabled);
  m_ui.plot->replot(); // PeakPicker might get hidden/shown
}

void ALCPeakFittingView::setPeakPicker(const IPeakFunction_const_sptr& peak)
{
  m_peakPicker->setPeak(peak);
  m_ui.plot->replot();
}

void ALCPeakFittingView::help()
{
  MantidQt::API::HelpWindow::showCustomInterface(NULL, QString("Muon_ALC"));
}

} // namespace CustomInterfaces
} // namespace Mantid

