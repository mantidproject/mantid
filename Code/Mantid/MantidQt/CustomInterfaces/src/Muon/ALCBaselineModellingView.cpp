#include "MantidQtCustomInterfaces/Muon/ALCBaselineModellingView.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/AlgorithmManager.h"

#include <boost/scoped_array.hpp>

#include <QMessageBox>

using namespace Mantid::API;

namespace MantidQt
{
namespace CustomInterfaces
{
  ALCBaselineModellingView::ALCBaselineModellingView(QWidget* widget)
    : m_widget(widget), m_ui(),
      m_dataCurve(new QwtPlotCurve()), m_fitCurve(new QwtPlotCurve()),
      m_correctedCurve(new QwtPlotCurve()), m_sectionSelector(NULL)
  {}
    
  void ALCBaselineModellingView::initialize()
  {
    m_ui.setupUi(m_widget);
    connect(m_ui.fit, SIGNAL(pressed()), SIGNAL(fit()));
    connect(m_ui.addSection, SIGNAL(pressed()), SLOT(onAddSectionPressed()));

    m_dataCurve->attach(m_ui.dataPlot);

    m_fitCurve->setPen(QPen(Qt::red));
    m_fitCurve->attach(m_ui.dataPlot);

    m_correctedCurve->attach(m_ui.correctedPlot);

    m_sectionSelector = new RangeSelector(m_ui.dataPlot);
    connect(m_sectionSelector, SIGNAL(selectionChanged(double,double)), this, SLOT(updateRange(double,double)));
  }

  IFunction_const_sptr ALCBaselineModellingView::function() const
  {
    return FunctionFactory::Instance().createInitialized(m_ui.function->text().toStdString());
  }

  void ALCBaselineModellingView::setData(MatrixWorkspace_const_sptr data)
  {
    m_dataCurve->setData(&data->readX(0)[0], &data->readY(0)[0], static_cast<int>(data->blocksize()));

    double xMin = data->getXMin();
    double xMax = data->getXMax();

    m_sectionSelector->setMaximum(xMin);
    m_sectionSelector->setMinimum(xMax);

    m_sectionSelector->setRange(xMin, xMax);

    m_ui.dataPlot->replot();
  }

  void ALCBaselineModellingView::setCorrectedData(MatrixWorkspace_const_sptr data)
  {
    m_correctedCurve->setData(&data->readX(0)[0], &data->readY(0)[0],
        static_cast<int>(data->blocksize()));

    m_ui.correctedPlot->replot();
  }

  void ALCBaselineModellingView::setFunction(IFunction_const_sptr func)
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
    m_ui.dataPlot->replot();

    m_ui.function->setText(QString::fromStdString(func->asString()));
  }

  void ALCBaselineModellingView::setSections(const std::vector<IALCBaselineModellingView::Section>& sections)
  {
    std::ostringstream sectionsStr;

    for (auto it = sections.begin(); it != sections.end(); ++it)
    {
      sectionsStr << it->first << " " << it->second << "\n";
    }

    m_ui.sections->setPlainText(QString::fromStdString(sectionsStr.str()));
  }

  void ALCBaselineModellingView::onAddSectionPressed()
  {
    QStringList range = m_ui.range->text().split(' ');
    assert(range.size() == 2);
    double from = range[0].toDouble();
    double to = range[1].toDouble();
    emit addSection(std::make_pair(from, to));
  }

  void ALCBaselineModellingView::updateRange(double min, double max)
  {
    m_ui.range->setText(QString("%1 %2").arg(min).arg(max));
  }

} // namespace CustomInterfaces
} // namespace MantidQt
