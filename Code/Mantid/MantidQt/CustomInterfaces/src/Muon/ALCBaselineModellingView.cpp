#include "MantidQtCustomInterfaces/Muon/ALCBaselineModellingView.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionDomain1D.h"

#include <boost/scoped_array.hpp>

using namespace Mantid::API;

namespace MantidQt
{
namespace CustomInterfaces
{
  ALCBaselineModellingView::ALCBaselineModellingView(QWidget* widget)
    : m_widget(widget), m_ui(),
      m_dataCurve(new QwtPlotCurve()), m_fitCurve(new QwtPlotCurve()),
      m_correctedCurve(new QwtPlotCurve())
  {}
    
  void ALCBaselineModellingView::initialize()
  {
    m_ui.setupUi(m_widget);
    connect(m_ui.fit, SIGNAL(pressed()), SIGNAL(fit()));

    m_dataCurve->attach(m_ui.dataPlot);

    m_fitCurve->setPen(QPen(Qt::red));
  }

  IFunction_const_sptr ALCBaselineModellingView::function() const
  {
    return FunctionFactory::Instance().createInitialized(m_ui.function->text().toStdString());
  }

  std::vector<IALCBaselineModellingView::Section> ALCBaselineModellingView::sections() const
  {
    std::istringstream sectionsStr(m_ui.sections->text().toStdString());
    double from,to;
    std::vector<Section> sections;

    while(sectionsStr >> from >> to)
    {
      sections.push_back(std::make_pair(from,to));
    }

    return sections;
  }

  // TODO: refactor out for other step views to use
  void ALCBaselineModellingView::displayData(MatrixWorkspace_const_sptr data)
  {
    const Mantid::MantidVec& dataX = data->readX(0);
    const Mantid::MantidVec& dataY = data->readY(0);

    m_dataCurve->setData(&dataX[0], &dataY[0], static_cast<int>(data->blocksize()));
    m_ui.dataPlot->replot();
  }

  void ALCBaselineModellingView::displayCorrected(MatrixWorkspace_const_sptr data)
  {
    const Mantid::MantidVec& dataX = data->readX(0);
    const Mantid::MantidVec& dataY = data->readY(0);

    m_correctedCurve->setData(&dataX[0], &dataY[0], static_cast<int>(data->blocksize()));
    m_correctedCurve->attach(m_ui.correctedPlot);
    m_ui.correctedPlot->replot();
  }

  void ALCBaselineModellingView::updateFunction(IFunction_const_sptr func)
  {
    std::vector<double> dataX;
    dataX.reserve(m_dataCurve->dataSize());

    for ( int i = 0; i < m_dataCurve->dataSize(); ++i )
    {
      dataX.push_back(m_dataCurve->x(i));
    }

    FunctionDomain1DVector domain(dataX);
    FunctionValues values(domain);

    func->function(domain, values);
    assert(values.size() > 0);

    m_fitCurve->setData(&dataX[0], values.getPointerToCalculated(0), m_dataCurve->dataSize());
    m_fitCurve->attach(m_ui.dataPlot);
    m_ui.dataPlot->replot();

    m_ui.function->setText(QString::fromStdString(func->asString()));
  }

} // namespace CustomInterfaces
} // namespace MantidQt
