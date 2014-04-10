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
    connect(m_ui.fit, SIGNAL(pressed()), SIGNAL(fitRequested()));

    m_dataCurve->attach(m_ui.dataPlot);

    m_fitCurve->setPen(QPen(Qt::red));
    m_fitCurve->attach(m_ui.dataPlot);

    m_correctedCurve->setPen(QPen(Qt::green));
    m_correctedCurve->attach(m_ui.correctedPlot);

    // Context menu for sections table
    m_ui.sections->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_ui.sections, SIGNAL(customContextMenuRequested(const QPoint &)),
            SLOT(sectionsContextMenu(const QPoint&)));

    // Make columns non-resizeable and to fill all the available space
    m_ui.sections->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
  }

  IFunction_const_sptr ALCBaselineModellingView::function() const
  {
    return m_ui.function->getFunction();
  }

  int ALCBaselineModellingView::sectionCount() const
  {
    return m_ui.sections->rowCount();
  }

  IALCBaselineModellingModel::Section ALCBaselineModellingView::section(int index) const
  {
    double start = m_ui.sections->item(index, SECTION_START_COL)->text().toDouble();
    double end = m_ui.sections->item(index, SECTION_END_COL)->text().toDouble();

    return IALCBaselineModellingModel::Section(start, end);
  }

  void ALCBaselineModellingView::setDataCurve(const QwtData &data)
  {
    m_dataCurve->setData(data);
    m_ui.dataPlot->replot();
  }

  void ALCBaselineModellingView::setCorrectedCurve(const QwtData &data)
  {
    m_correctedCurve->setData(data);
    m_ui.correctedPlot->replot();
  }

  void ALCBaselineModellingView::setBaselineCurve(const QwtData &data)
  {
    m_fitCurve->setData(data);
    m_ui.dataPlot->replot();
  }

  void ALCBaselineModellingView::setFunction(IFunction_const_sptr func)
  {
    m_ui.function->setFunction(QString::fromStdString(func->asString()));
  }

  void ALCBaselineModellingView::addSection(IALCBaselineModellingModel::Section newSection)
  {
    int newIndex = m_ui.sections->rowCount();

    m_ui.sections->insertRow(newIndex);

    m_ui.sections->setItem(newIndex, SECTION_START_COL,
                           new QTableWidgetItem(QString::number(newSection.first)));
    m_ui.sections->setItem(newIndex, SECTION_END_COL,
                           new QTableWidgetItem(QString::number(newSection.second)));
  }

  void ALCBaselineModellingView::sectionsContextMenu(const QPoint& widgetPoint)
  {
    UNUSED_ARG(widgetPoint);

    QMenu context(m_widget);
    context.addAction("Add section", this, SIGNAL(addSectionRequested()));
    context.exec(QCursor::pos());
  }

} // namespace CustomInterfaces
} // namespace MantidQt
