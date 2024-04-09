// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALCBaselineModellingPresenter.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace.h"

using namespace Mantid::API;

namespace MantidQt::CustomInterfaces {

ALCBaselineModellingPresenter::ALCBaselineModellingPresenter(IALCBaselineModellingView *view,
                                                             IALCBaselineModellingModel *model)
    : m_view(view), m_model(model) {}

void ALCBaselineModellingPresenter::initialize() {
  m_view->initialize();

  // View actions
  connect(m_view, SIGNAL(fitRequested()), SLOT(fit()));
  connect(m_view, SIGNAL(addSectionRequested()), SLOT(addSection()));
  connect(m_view, SIGNAL(removeSectionRequested(int)), SLOT(removeSection(int)));

  // View events (sync)
  connect(m_view, SIGNAL(sectionRowModified(int)), SLOT(onSectionRowModified(int)));
  connect(m_view, SIGNAL(sectionSelectorModified(int)), SLOT(onSectionSelectorModified(int)));

  // Model updates
  connect(m_model, SIGNAL(dataChanged()), SLOT(updateDataCurve()));
  connect(m_model, SIGNAL(correctedDataChanged()), SLOT(updateCorrectedCurve()));
  connect(m_model, SIGNAL(fittedFunctionChanged()), SLOT(updateFunction()));
  connect(m_model, SIGNAL(fittedFunctionChanged()), SLOT(updateBaselineCurve()));
}

/**
 * Perform a fit and updates the view accordingly
 */
void ALCBaselineModellingPresenter::fit() {
  std::vector<IALCBaselineModellingModel::Section> parsedSections;

  for (int i = 0; i < m_view->noOfSectionRows(); ++i) {
    auto sectionRow = m_view->sectionRow(i);

    double min = sectionRow.first.toDouble();
    double max = sectionRow.second.toDouble();

    IALCBaselineModellingModel::Section parsedSection(min, max);

    parsedSections.emplace_back(parsedSection);
  }

  std::string funcStr = m_view->function();

  if (funcStr.empty()) {
    m_view->displayError("Couldn't fit an empty function");
  } else if (parsedSections.empty()) {
    m_view->displayError("No sections to fit");
  } else {
    try {
      IFunction_sptr funcToFit = FunctionFactory::Instance().createInitialized(funcStr);
      m_model->fit(funcToFit, parsedSections);
    } catch (std::exception &e) {
      m_view->displayError(QString::fromStdString(e.what()));
    }
  }
}

/**
 * Adds new section in the view
 */
void ALCBaselineModellingPresenter::addSection() {
  if (MatrixWorkspace_const_sptr data = m_model->data()) {
    double xMin = data->getXMin();
    double xMax = data->getXMax();

    int noOfSections = m_view->noOfSectionRows();

    m_view->setNoOfSectionRows(noOfSections + 1);

    m_view->setSectionRow(noOfSections, std::make_pair(QString::number(xMin), QString::number(xMax)));

    m_view->addSectionSelector(noOfSections, std::make_pair(xMin, xMax));
  } else {
    m_view->displayError("Please load some data first");
  }
}

/**
 * @param row :: Section row to remove
 */
void ALCBaselineModellingPresenter::removeSection(int row) {
  // The view should make sure the row is valid
  assert(row >= 0);
  assert(row < m_view->noOfSectionRows());

  // Delete all section selectors
  for (int i = 0; i < m_view->noOfSectionRows(); ++i) {
    m_view->deleteSectionSelector(i);
  }

  std::vector<IALCBaselineModellingView::SectionRow> allRows;

  for (int i = 0; i < m_view->noOfSectionRows(); ++i) {
    allRows.emplace_back(m_view->sectionRow(i));
  }

  allRows.erase(allRows.begin() + row);

  // Shrink sections table
  m_view->setNoOfSectionRows(static_cast<int>(allRows.size()));

  // Update row values and add sections selectors
  for (size_t i = 0; i < allRows.size(); ++i) {
    m_view->setSectionRow(static_cast<int>(i), allRows[i]);

    double startX = allRows[i].first.toDouble();
    double endX = allRows[i].second.toDouble();

    IALCBaselineModellingView::SectionSelector newSelector(startX, endX);
    m_view->addSectionSelector(static_cast<int>(i), newSelector);
  }
}

void ALCBaselineModellingPresenter::onSectionRowModified(int row) {
  auto sectionRow = m_view->sectionRow(row);

  double startX = sectionRow.first.toDouble();
  double endX = sectionRow.second.toDouble();

  int index(row); // That's what we make sure of in addSection()
  IALCBaselineModellingView::SectionSelector selector(startX, endX);

  m_view->updateSectionSelector(index, selector);
}

/**
 * @param index :: Index of section selector
 */
void ALCBaselineModellingPresenter::onSectionSelectorModified(int index) {
  auto selectorValues = m_view->sectionSelector(index);

  QString startX = QString::number(selectorValues.first);
  QString endX = QString::number(selectorValues.second);

  int row(index); // That's what we make sure of in addSection()
  IALCBaselineModellingView::SectionRow rowValues(startX, endX);

  m_view->setSectionRow(row, rowValues);
}

void ALCBaselineModellingPresenter::updateDataCurve() {
  if (auto dataWs = m_model->data()) {
    m_view->setDataCurve(dataWs);
    // Delete all section selectors
    int noRows = m_view->noOfSectionRows() - 1;
    for (int j = noRows; j > -1; --j) {
      removeSection(j);
    }
  }
}

void ALCBaselineModellingPresenter::updateCorrectedCurve() {
  if (auto correctedData = m_model->correctedData())
    m_view->setCorrectedCurve(correctedData);
  else
    m_view->removePlot("Corrected");
}

void ALCBaselineModellingPresenter::updateBaselineCurve() {
  if (const auto fitFunction = m_model->fittedFunction()) {
    const auto &xValues = m_model->data()->x(0);
    const auto baslineWorkspace = m_model->baselineData(fitFunction, xValues.rawData());
    m_view->setBaselineCurve(baslineWorkspace);
  } else {
    m_view->removePlot("Baseline");
  }
}

void ALCBaselineModellingPresenter::updateFunction() {
  if (IFunction_const_sptr fittedFunc = m_model->fittedFunction()) {
    m_view->setFunction(fittedFunc);
  } else {
    m_view->setFunction(IFunction_const_sptr());
  }
}

} // namespace MantidQt::CustomInterfaces
