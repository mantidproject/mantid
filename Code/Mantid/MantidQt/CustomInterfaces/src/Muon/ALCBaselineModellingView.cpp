#include "MantidQtCustomInterfaces/Muon/ALCBaselineModellingView.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/AlgorithmManager.h"

#include <boost/scoped_array.hpp>

#include <QMessageBox>
#include <QMenu>

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
    m_fitCurve->attach(m_ui.dataPlot);

    m_correctedCurve->attach(m_ui.correctedPlot);

    // Context menu for sections table
    m_ui.sections->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_ui.sections, SIGNAL(customContextMenuRequested(const QPoint &)),
            SLOT(sectionsContextMenu(const QPoint&)));

    // Make columns non-resizeable and to fill all the available space
    m_ui.sections->horizontalHeader()->setResizeMode(QHeaderView::Stretch);

    connect(m_ui.sections, SIGNAL(cellChanged(int,int)), SLOT(onSectionChanged(int,int)));
  }

  IFunction_const_sptr ALCBaselineModellingView::function() const
  {
    return FunctionFactory::Instance().createInitialized(m_ui.function->text().toStdString());
  }

  void ALCBaselineModellingView::setData(MatrixWorkspace_const_sptr data)
  {
    m_dataCurve->setData(&data->readX(0)[0], &data->readY(0)[0], static_cast<int>(data->blocksize()));

    m_ui.dataPlot->replot();
  }

  void ALCBaselineModellingView::setCorrectedData(MatrixWorkspace_const_sptr data)
  {
    m_correctedCurve->setData(&data->readX(0)[0], &data->readY(0)[0], static_cast<int>(data->blocksize()));

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
    // We disable table signals so that cell update signals are not emitted. This causes problems
    // when the table is half filled
    bool prevBlockedState = m_ui.sections->blockSignals(true);

    m_ui.sections->setRowCount(static_cast<int>(sections.size()));

    for (auto it = sections.begin(); it != sections.end(); ++it)
    {
      int row = static_cast<int>(std::distance(sections.begin(), it));

      m_ui.sections->setItem(row, SECTION_START_COL,
                             new QTableWidgetItem(QString::number(it->first)));

      m_ui.sections->setItem(row, SECTION_END_COL,
                             new QTableWidgetItem(QString::number(it->second)));
    }

   m_ui.sections->blockSignals(prevBlockedState);
  }

  void ALCBaselineModellingView::sectionsContextMenu(const QPoint& widgetPoint)
  {
    UNUSED_ARG(widgetPoint);

    QMenu context(m_widget);
    context.addAction("Add section", this, SLOT(requestAddSection()));
    context.exec(QCursor::pos());
  }

  void ALCBaselineModellingView::requestAddSection()
  {
    emit addSection(std::make_pair(0,0));
  }

  void ALCBaselineModellingView::onSectionChanged(int row, int col)
  {
    UNUSED_ARG(col); // We are updating both values at once anyway

    assert(row >= 0); // I assume Qt handles that

    double start = m_ui.sections->item(row, SECTION_START_COL)->text().toDouble();
    double end = m_ui.sections->item(row, SECTION_END_COL)->text().toDouble();

    emit modifySection(static_cast<SectionIndex>(row), std::make_pair(start, end));
  }

} // namespace CustomInterfaces
} // namespace MantidQt
