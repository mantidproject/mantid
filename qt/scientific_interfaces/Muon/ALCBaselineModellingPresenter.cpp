// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALCBaselineModellingPresenter.h"

#include "IALCBaselineModellingView.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace.h"

using namespace Mantid::API;

namespace MantidQt::CustomInterfaces {

ALCBaselineModellingPresenter::ALCBaselineModellingPresenter(IALCBaselineModellingView *view,
                                                             std::unique_ptr<IALCBaselineModellingModel> model)
    : m_view(view), m_model(std::move(model)) {}

void ALCBaselineModellingPresenter::initialize() {
  m_view->initialize();
  m_view->subscribePresenter(this);
}

void ALCBaselineModellingPresenter::subscribe(IALCBaselineModellingPresenterSubscriber *subscriber) {
  m_subscriber = subscriber;
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
      updateAfterFit();
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
  if (auto correctedDataWs = m_model->correctedData())
    m_view->setCorrectedCurve(correctedDataWs);
  else
    m_view->removePlot("Corrected");
  m_subscriber->correctedDataChanged();
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

void ALCBaselineModellingPresenter::updateAfterFit() {
  updateCorrectedCurve();
  updateFunction();
  updateBaselineCurve();
}

Mantid::API::MatrixWorkspace_sptr ALCBaselineModellingPresenter::exportWorkspace() const {
  return m_model->exportWorkspace();
}

Mantid::API::ITableWorkspace_sptr ALCBaselineModellingPresenter::exportSections() const {
  return m_model->exportSections();
}

Mantid::API::ITableWorkspace_sptr ALCBaselineModellingPresenter::exportModel() const { return m_model->exportModel(); }

Mantid::API::MatrixWorkspace_sptr ALCBaselineModellingPresenter::correctedData() const {
  return m_model->correctedData();
}

void ALCBaselineModellingPresenter::setData(Mantid::API::MatrixWorkspace_sptr data) {
  m_model->setData(data);
  updateDataCurve();
}

void ALCBaselineModellingPresenter::setCorrectedData(Mantid::API::MatrixWorkspace_sptr data) {
  m_model->setCorrectedData(data);
  updateCorrectedCurve();
}

std::string ALCBaselineModellingPresenter::function() const { return m_view->function(); }

int ALCBaselineModellingPresenter::noOfSectionRows() const { return m_view->noOfSectionRows(); }

} // namespace MantidQt::CustomInterfaces
