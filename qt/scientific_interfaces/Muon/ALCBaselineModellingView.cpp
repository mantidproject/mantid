// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALCBaselineModellingView.h"

#include "ALCBaselineModellingPresenter.h"
#include "MantidQtWidgets/Common/HelpWindow.h"

#include <QMenu>
#include <QMessageBox>
#include <QSignalMapper>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

namespace MantidQt::CustomInterfaces {
ALCBaselineModellingView::ALCBaselineModellingView(QWidget *widget)
    : m_widget(widget), m_ui(), m_rangeSelectors(), m_selectorModifiedMapper(new QSignalMapper(this)) {}

ALCBaselineModellingView::~ALCBaselineModellingView() = default;

void ALCBaselineModellingView::initialize() {
  m_ui.setupUi(m_widget);
  connect(m_ui.fit, SIGNAL(clicked()), SLOT(handleFitRequested()));

  m_ui.dataPlot->setCanvasColour(Qt::white);
  m_ui.correctedPlot->setCanvasColour(Qt::white);

  // Error bars on the plot
  const QStringList dataPlotErrors{"Data"};
  m_ui.dataPlot->setLinesWithErrors(dataPlotErrors);
  const QStringList correctedPlotErrors{"Corrected"};
  m_ui.correctedPlot->setLinesWithErrors(correctedPlotErrors);

  // Context menu for sections table
  m_ui.sections->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_ui.sections, SIGNAL(customContextMenuRequested(const QPoint &)), SLOT(sectionsContextMenu(const QPoint &)));

  // Make columns non-resizeable and to fill all the available space
  m_ui.sections->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

  connect(m_ui.sections, SIGNAL(cellChanged(int, int)), SIGNAL(sectionRowModified(int)));

  connect(m_selectorModifiedMapper, SIGNAL(mapped(int)), SIGNAL(sectionSelectorModified(int)));

  connect(m_ui.help, SIGNAL(clicked()), this, SLOT(help()));

  initConnections();
}

void ALCBaselineModellingView::initConnections() const {
  // View actions
  connect(this, SIGNAL(fitRequested()), SLOT(handleFitRequested()));
  connect(this, SIGNAL(addSectionRequested()), SLOT(handleAddSectionRequested()));
  connect(this, SIGNAL(removeSectionRequested(int)), SLOT(handleRemoveSectionRequested(int)));

  // View events (sync)
  connect(this, SIGNAL(sectionRowModified(int)), SLOT(handleSectionRowModified(int)));
  connect(this, SIGNAL(sectionSelectorModified(int)), SLOT(handleSectionSelectorModified(int)));
}

std::string ALCBaselineModellingView::function() const { return m_ui.function->getFunctionString(); }

IALCBaselineModellingView::SectionRow ALCBaselineModellingView::sectionRow(int row) const {
  QString first = m_ui.sections->item(row, 0)->text();
  QString second = m_ui.sections->item(row, 1)->text();
  return SectionRow(first, second);
}

IALCBaselineModellingView::SectionSelector ALCBaselineModellingView::sectionSelector(int index) const {
  auto rangeSelector = m_rangeSelectors.find(index)->second;
  return std::make_pair(rangeSelector->getMinimum(), rangeSelector->getMaximum());
}

int ALCBaselineModellingView::noOfSectionRows() const { return m_ui.sections->rowCount(); }

void ALCBaselineModellingView::setDataCurve(MatrixWorkspace_sptr workspace, std::size_t const &workspaceIndex) {
  const auto kwargs = getPlotKwargs(m_ui.dataPlot, "Data");

  m_ui.dataPlot->clear();
  m_ui.dataPlot->addSpectrum("Data", workspace, workspaceIndex, Qt::black, kwargs);
}

void ALCBaselineModellingView::setCorrectedCurve(MatrixWorkspace_sptr workspace, std::size_t const &workspaceIndex) {
  const auto kwargs = getPlotKwargs(m_ui.correctedPlot, "Corrected");

  m_ui.correctedPlot->clear();
  m_ui.correctedPlot->addSpectrum("Corrected", workspace, workspaceIndex, Qt::blue, kwargs);
}

QHash<QString, QVariant> ALCBaselineModellingView::getPlotKwargs(PreviewPlot *plot, const QString &curveName) {
  // Ensures the plot is plotted only with data points and no lines
  QHash<QString, QVariant> kwargs;
  UNUSED_ARG(plot);
  UNUSED_ARG(curveName);
  kwargs.insert("linestyle", QString("None").toLatin1().constData());
  kwargs.insert("marker", QString(".").toLatin1().constData());
  kwargs.insert("distribution", QString("False").toLatin1().constData());

  return kwargs;
}

void ALCBaselineModellingView::setBaselineCurve(MatrixWorkspace_sptr workspace, std::size_t const &workspaceIndex) {
  m_ui.dataPlot->addSpectrum("Baseline", workspace, workspaceIndex, Qt::red);
  m_ui.dataPlot->replot();
}

void ALCBaselineModellingView::removePlot(QString const &plotName) {
  m_ui.dataPlot->removeSpectrum(plotName);
  m_ui.dataPlot->replot();
}

void ALCBaselineModellingView::setFunction(IFunction_const_sptr func) {
  if (!func) {
    m_ui.function->clear();
  } else {
    size_t nParams = func->nParams();
    for (size_t i = 0; i < nParams; i++) {

      auto name = func->parameterName(i);
      double value = func->getParameter(i);
      double error = func->getError(i);

      m_ui.function->setParameter(name, value);
      m_ui.function->setParameterError(name, error);
    }
  }
}

void ALCBaselineModellingView::setNoOfSectionRows(int rows) { m_ui.sections->setRowCount(rows); }

void ALCBaselineModellingView::setSectionRow(int row, IALCBaselineModellingView::SectionRow values) {
  m_ui.sections->blockSignals(true); // Setting values, no need for 'modified' signals
  m_ui.sections->setFocus();
  m_ui.sections->selectRow(row);
  m_ui.sections->setItem(row, 0, new QTableWidgetItem(values.first));
  m_ui.sections->setItem(row, 1, new QTableWidgetItem(values.second));
  m_ui.sections->blockSignals(false);
}

void ALCBaselineModellingView::addSectionSelector(int index, IALCBaselineModellingView::SectionSelector values) {
  auto *newSelector = new RangeSelector(m_ui.dataPlot);

  if (index % 3 == 0) {
    newSelector->setColour(Qt::blue);
  } else if ((index - 1) % 3 == 0) {
    newSelector->setColour(Qt::red);
  } else {
    newSelector->setColour(Qt::green);
  }

  m_selectorModifiedMapper->setMapping(newSelector, index);
  connect(newSelector, SIGNAL(selectionChanged(double, double)), m_selectorModifiedMapper, SLOT(map()));

  m_rangeSelectors[index] = newSelector;

  // Set initial values
  newSelector->setRange(values.first, values.second);
  newSelector->setBounds(values.first, values.second);
  setSelectorValues(newSelector, values);

  m_ui.dataPlot->replot();
}

void ALCBaselineModellingView::deleteSectionSelector(int index) {
  auto rangeSelector = m_rangeSelectors[index];
  m_rangeSelectors.erase(index);

  rangeSelector->detach(); // This is not done when it's deleted
  m_selectorModifiedMapper->removeMappings(rangeSelector);
  delete rangeSelector;
}

void ALCBaselineModellingView::updateSectionSelector(int index, IALCBaselineModellingView::SectionSelector values) {
  setSelectorValues(m_rangeSelectors[index], values);
}

void ALCBaselineModellingView::displayError(const QString &message) {
  QMessageBox::critical(m_widget, "Error", message);
}

void ALCBaselineModellingView::sectionsContextMenu(const QPoint &widgetPoint) {
  QMenu menu(m_widget);
  menu.addAction("Add section", this, SIGNAL(addSectionRequested()));

  // Helper mapper to map removal action to row id
  QSignalMapper removalActionMapper;
  connect(&removalActionMapper, SIGNAL(mapped(int)), SIGNAL(removeSectionRequested(int)));

  int row = m_ui.sections->rowAt(widgetPoint.y());
  if (row != -1) {
    // Add removal action
    QAction *removeAction = menu.addAction("Remove section", &removalActionMapper, SLOT(map()));
    removalActionMapper.setMapping(removeAction, row);
  }

  menu.exec(QCursor::pos());
}

void ALCBaselineModellingView::setSelectorValues(RangeSelector *selector,
                                                 IALCBaselineModellingView::SectionSelector values) {
  // if the values are not increasing then reverse them
  if (values.first > values.second) {
    const double tempSwapValue = values.first;
    values.first = values.second;
    values.second = tempSwapValue;
  }
  selector->setMinimum(values.first);
  selector->setMaximum(values.second);
}

void ALCBaselineModellingView::help() {
  MantidQt::API::HelpWindow::showCustomInterface(QString("Muon ALC"), QString("muon"));
}

void ALCBaselineModellingView::handleFitRequested() const { m_presenter->fit(); }

void ALCBaselineModellingView::handleAddSectionRequested() const { m_presenter->addSection(); }

void ALCBaselineModellingView::handleRemoveSectionRequested(int row) const { m_presenter->removeSection(row); }

void ALCBaselineModellingView::handleSectionRowModified(int row) const { m_presenter->onSectionRowModified(row); }

void ALCBaselineModellingView::handleSectionSelectorModified(int index) const {
  m_presenter->onSectionSelectorModified(index);
}

} // namespace MantidQt::CustomInterfaces
