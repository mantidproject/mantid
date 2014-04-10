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
      m_correctedCurve(new QwtPlotCurve()), m_rangeSelectors()
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

  std::vector<IALCBaselineModellingView::Section> ALCBaselineModellingView::sections() const
  {
    std::vector<Section> sections;

    for (int row = 0; row < m_ui.sections->rowCount(); ++row)
    {
      double start = m_ui.sections->item(row, SECTION_START_COL)->text().toDouble();
      double end = m_ui.sections->item(row, SECTION_END_COL)->text().toDouble();
      sections.push_back(Section(start,end));
    }

    return sections;
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

  void ALCBaselineModellingView::setSections(const std::vector<IALCBaselineModellingView::Section>& sections)
  {
    m_ui.sections->setRowCount(static_cast<int>(sections.size()));

    for(auto it = sections.begin(); it != sections.end(); ++it)
    {
      int row = static_cast<int>(std::distance(sections.begin(), it));
      setSectionRow(row, *it);
    }
  }

  void ALCBaselineModellingView::updateSection(size_t index, IALCBaselineModellingView::Section section)
  {
    setSectionRow(static_cast<int>(index), section);
  }

  void ALCBaselineModellingView::setSectionSelectors(
      const std::vector<IALCBaselineModellingView::SectionSelector>& selectors)
  {
    // Delete previous range selectors
    for (auto it = m_rangeSelectors.begin(); it != m_rangeSelectors.end(); ++it)
    {
      delete *it;
    }
    m_rangeSelectors.clear();

    // Create required range selectors
    for (auto it = selectors.begin(); it != selectors.end(); ++it)
    {
      auto selector = new RangeSelector(m_ui.dataPlot);
      selector->setRange(*it);
      selector->setMinimum(it->first);
      selector->setMaximum(it->second);
      connect(selector, SIGNAL(selectionChanged(double,double)),
              SLOT(onRangeSelectorChanged(double,double)));
      m_rangeSelectors.push_back(selector);
    }
  }

  void ALCBaselineModellingView::sectionsContextMenu(const QPoint& widgetPoint)
  {
    UNUSED_ARG(widgetPoint);

    QMenu context(m_widget);
    context.addAction("Add section", this, SIGNAL(addSectionRequested()));
    context.exec(QCursor::pos());
  }

  void ALCBaselineModellingView::onRangeSelectorChanged(double min, double max)
  {
    auto sender = QObject::sender();
    auto rangeSelector = dynamic_cast<RangeSelector*>(sender);
    assert(rangeSelector); // If everything was connected propertly

    auto it = std::find(m_rangeSelectors.begin(), m_rangeSelectors.end(), rangeSelector);
    assert(it != m_rangeSelectors.end()); // Range selector wasn't added to the array

    size_t index = std::distance(m_rangeSelectors.begin(), it);

    emit sectionSelectorModified(index, min, max);
  }

  void ALCBaselineModellingView::setSectionRow(int row, IALCBaselineModellingView::Section section)
  {
      m_ui.sections->setItem(row, SECTION_START_COL,
                             new QTableWidgetItem(QString::number(section.first)));
      m_ui.sections->setItem(row, SECTION_END_COL,
                             new QTableWidgetItem(QString::number(section.second)));
  }

} // namespace CustomInterfaces
} // namespace MantidQt
