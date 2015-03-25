#include "MantidQtCustomInterfaces/Muon/ALCBaselineModellingView.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/AlgorithmManager.h"

#include <boost/scoped_array.hpp>

#include <QDesktopServices>
#include <QMessageBox>
#include <QMenu>
#include <QSignalMapper>
#include <QUrl>

#include <qwt_symbol.h>

using namespace Mantid::API;

namespace MantidQt
{
namespace CustomInterfaces
{
  ALCBaselineModellingView::ALCBaselineModellingView(QWidget* widget)
    : m_widget(widget), m_ui(),
      m_dataCurve(new QwtPlotCurve()), m_fitCurve(new QwtPlotCurve()),
      m_correctedCurve(new QwtPlotCurve()), m_rangeSelectors(),
      m_selectorModifiedMapper(new QSignalMapper(this))
  {}
    
  void ALCBaselineModellingView::initialize()
  {
    m_ui.setupUi(m_widget);
    connect(m_ui.fit, SIGNAL(clicked()), SIGNAL(fitRequested()));

    m_ui.dataPlot->setCanvasBackground(Qt::white);
    m_ui.dataPlot->setAxisFont(QwtPlot::xBottom, m_widget->font());
    m_ui.dataPlot->setAxisFont(QwtPlot::yLeft, m_widget->font());

    m_ui.correctedPlot->setCanvasBackground(Qt::white);
    m_ui.correctedPlot->setAxisFont(QwtPlot::xBottom, m_widget->font());
    m_ui.correctedPlot->setAxisFont(QwtPlot::yLeft, m_widget->font());

    m_dataCurve->setStyle(QwtPlotCurve::NoCurve);
    m_dataCurve->setSymbol(QwtSymbol(QwtSymbol::Ellipse, QBrush(), QPen(), QSize(7,7)));
    m_dataCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    m_dataCurve->attach(m_ui.dataPlot);

    m_fitCurve->setPen(QPen(Qt::red, 1.5));
    m_fitCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    m_fitCurve->attach(m_ui.dataPlot);

    m_correctedCurve->setStyle(QwtPlotCurve::NoCurve);
    m_correctedCurve->setSymbol(QwtSymbol(QwtSymbol::Ellipse, QBrush(), QPen(Qt::green), QSize(7,7)));
    m_correctedCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    m_correctedCurve->attach(m_ui.correctedPlot);

    // Context menu for sections table
    m_ui.sections->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_ui.sections, SIGNAL(customContextMenuRequested(const QPoint &)),
            SLOT(sectionsContextMenu(const QPoint&)));

    // Make columns non-resizeable and to fill all the available space
    m_ui.sections->horizontalHeader()->setResizeMode(QHeaderView::Stretch);

    connect(m_ui.sections, SIGNAL(cellChanged(int,int)), SIGNAL(sectionRowModified(int)));

    connect(m_selectorModifiedMapper, SIGNAL(mapped(int)), SIGNAL(sectionSelectorModified(int)));

    connect(m_ui.help, SIGNAL(clicked()), this, SLOT(help()));
  }

  QString ALCBaselineModellingView::function() const
  {
    return m_ui.function->getFunctionString();
  }

  IALCBaselineModellingView::SectionRow ALCBaselineModellingView::sectionRow(int row) const
  {
    QString first = m_ui.sections->item(row, 0)->text();
    QString second = m_ui.sections->item(row, 1)->text();
    return SectionRow(first, second);
  }

  IALCBaselineModellingView::SectionSelector ALCBaselineModellingView::sectionSelector(int index) const
  {
    auto rangeSelector = m_rangeSelectors.find(index)->second;
    return std::make_pair(rangeSelector->getMinimum(), rangeSelector->getMaximum());
  }

  int ALCBaselineModellingView::noOfSectionRows() const
  {
    return m_ui.sections->rowCount();
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

  void ALCBaselineModellingView::setFunction(const QString& func)
  {
    if (func.isEmpty())
    {
      m_ui.function->clear();
    }
    else
    {
      m_ui.function->setFunction(func);
    }
  }

  void ALCBaselineModellingView::setNoOfSectionRows(int rows)
  {
    m_ui.sections->setRowCount(rows);
  }

  void ALCBaselineModellingView::setSectionRow(int row, IALCBaselineModellingView::SectionRow values)
  {
    m_ui.sections->blockSignals(true); // Setting values, no need for 'modified' signals
    m_ui.sections->setItem(row, 0, new QTableWidgetItem(values.first));
    m_ui.sections->setItem(row, 1, new QTableWidgetItem(values.second));
    m_ui.sections->blockSignals(false);
  }

  void ALCBaselineModellingView::addSectionSelector(int index,
                                                    IALCBaselineModellingView::SectionSelector values)
  {
    RangeSelector* newSelector = new RangeSelector(m_ui.dataPlot);

    m_selectorModifiedMapper->setMapping(newSelector,index);
    connect(newSelector, SIGNAL(selectionChanged(double,double)),
            m_selectorModifiedMapper, SLOT(map()));

    m_rangeSelectors[index] = newSelector;

    // Set initial values
    setSelectorValues(newSelector, values);

    m_ui.dataPlot->replot();
  }

  void ALCBaselineModellingView::deleteSectionSelector(int index)
  {
    auto rangeSelector = m_rangeSelectors[index];
    m_rangeSelectors.erase(index);

    rangeSelector->detach(); // This is not done when it's deleted
    m_selectorModifiedMapper->removeMappings(rangeSelector);
    delete rangeSelector;

    m_ui.dataPlot->replot();
  }

  void ALCBaselineModellingView::updateSectionSelector(int index,
                                                       IALCBaselineModellingView::SectionSelector values)
  {
    setSelectorValues(m_rangeSelectors[index], values);
  }

  void ALCBaselineModellingView::displayError(const QString& message)
  {
    QMessageBox::critical(m_widget, "Error", message);
  }

  void ALCBaselineModellingView::sectionsContextMenu(const QPoint& widgetPoint)
  {
    QMenu menu(m_widget);
    menu.addAction("Add section", this, SIGNAL(addSectionRequested()));

    // Helper mapper to map removal action to row id
    QSignalMapper removalActionMapper;
    connect(&removalActionMapper, SIGNAL(mapped(int)), SIGNAL(removeSectionRequested(int)));

    int row = m_ui.sections->rowAt(widgetPoint.y());
    if (row != -1)
    {
      // Add removal action
      QAction* removeAction = menu.addAction("Remove section", &removalActionMapper, SLOT(map()));
      removalActionMapper.setMapping(removeAction, row);
    }

    menu.exec(QCursor::pos());
  }

  void ALCBaselineModellingView::setSelectorValues(RangeSelector* selector,
                                                   IALCBaselineModellingView::SectionSelector values)
  {
    // TODO: range sould be set to something meaningful
    selector->setRange(std::numeric_limits<double>::min(), std::numeric_limits<double>::max());

    selector->setMinimum(values.first);
    selector->setMaximum(values.second);
  }

  void ALCBaselineModellingView::help() {
    QDesktopServices::openUrl(QUrl(QString("http://www.mantidproject.org/") +
            "Muon_ALC:_Baseline_Modelling"));
  }

} // namespace CustomInterfaces
} // namespace MantidQt
