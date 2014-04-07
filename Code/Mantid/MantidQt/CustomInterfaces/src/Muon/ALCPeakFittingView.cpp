#include "MantidQtCustomInterfaces/Muon/ALCPeakFittingView.h"

#include "MantidAPI/FunctionFactory.h"

using namespace Mantid;

namespace MantidQt
{
namespace CustomInterfaces
{

ALCPeakFittingView::ALCPeakFittingView(QWidget* widget)
  : m_widget(widget), m_ui(), m_dataCurve(new QwtPlotCurve()), m_peakCurve(new QwtPlotCurve())
{}

IALCPeakFittingView::ListOfPeaks ALCPeakFittingView::peaks() const
{
  std::istringstream peaksStr(m_ui.peaks->toPlainText().toStdString());
  ListOfPeaks peaks;

  std::string peakStr;

  while(std::getline(peaksStr, peakStr))
  {
    IFunction_const_sptr peakFunc = API::FunctionFactory::Instance().createInitialized(peakStr);
    auto peak = boost::dynamic_pointer_cast<const IPeakFunction>(peakFunc);
    peaks.push_back(peak);
  }

  return peaks;
}

void ALCPeakFittingView::initialize()
{
  m_ui.setupUi(m_widget);

  connect(m_ui.fit, SIGNAL(pressed()), this, SIGNAL(fit()));

  m_dataCurve->attach(m_ui.plot);

  m_peakCurve->setPen(QPen(Qt::red));
  m_peakCurve->attach(m_ui.plot);
}

void ALCPeakFittingView::setData(MatrixWorkspace_const_sptr data)
{
  m_dataCurve->setData(&data->readX(0)[0], &data->readY(0)[0], static_cast<int>(data->blocksize()));
  m_ui.plot->replot();
}

void ALCPeakFittingView::setPeaks(const IALCPeakFittingView::ListOfPeaks& peaks)
{
  assert(peaks.size() == 1); // TODO: for now

  std::ostringstream peaksStr;

  for (auto it = peaks.begin(); it != peaks.end(); ++it)
  {
    peaksStr << (*it)->asString() << "\n";
  }

  m_ui.peaks->setPlainText(QString::fromStdString(peaksStr.str()));

  std::vector<double> dataX;
  dataX.reserve(m_dataCurve->dataSize());

  for ( int i = 0; i < m_dataCurve->dataSize(); ++i )
  {
    dataX.push_back(m_dataCurve->x(i));
  }

  FunctionDomain1DVector domain(dataX);
  FunctionValues values(domain);

  peaks[0]->function(domain, values);
  assert(values.size() > 0);

  m_peakCurve->setData(&dataX[0], values.getPointerToCalculated(0), m_dataCurve->dataSize());
  m_ui.plot->replot();
}

} // namespace CustomInterfaces
} // namespace Mantid

