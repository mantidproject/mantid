// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALCBaselineModellingModel.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"

#include "Poco/ActiveResult.h"
#include <QApplication>
#include <algorithm>
#include <utility>

using namespace Mantid::API;

namespace {

MatrixWorkspace_sptr extractSpectrum(const MatrixWorkspace_sptr &inputWorkspace, const int workspaceIndex) {
  auto extracter = AlgorithmManager::Instance().create("ExtractSingleSpectrum");
  extracter->setAlwaysStoreInADS(false);
  extracter->setProperty("InputWorkspace", inputWorkspace);
  extracter->setProperty("WorkspaceIndex", workspaceIndex);
  extracter->setPropertyValue("OutputWorkspace", "__NotUsed__");
  extracter->execute();
  MatrixWorkspace_sptr output = extracter->getProperty("OutputWorkspace");
  return output;
}

MatrixWorkspace_sptr evaluateFunction(const IFunction_const_sptr &function,
                                      const MatrixWorkspace_sptr &inputWorkspace) {
  auto fit = AlgorithmManager::Instance().create("Fit");
  fit->setAlwaysStoreInADS(false);
  fit->setProperty("Function", function->asString());
  fit->setProperty("InputWorkspace", inputWorkspace);
  fit->setProperty("MaxIterations", 0);
  fit->setProperty("CreateOutput", true);
  fit->execute();
  MatrixWorkspace_sptr output = fit->getProperty("OutputWorkspace");
  return output;
}

} // namespace

namespace MantidQt::CustomInterfaces {

void ALCBaselineModellingModel::fit(IFunction_const_sptr function, const std::vector<Section> &sections) {
  // Create a copy of the data
  IAlgorithm_sptr clone = AlgorithmManager::Instance().create("CloneWorkspace");
  clone->setAlwaysStoreInADS(false);
  clone->setProperty("InputWorkspace", std::const_pointer_cast<MatrixWorkspace>(m_data));
  clone->setProperty("OutputWorkspace", "__NotUsed__");
  clone->execute();

  Workspace_sptr cloned = clone->getProperty("OutputWorkspace");
  MatrixWorkspace_sptr dataToFit = std::dynamic_pointer_cast<MatrixWorkspace>(cloned);
  assert(dataToFit); // CloneWorkspace should take care of that

  disableUnwantedPoints(dataToFit, sections);

  IFunction_sptr funcToFit = FunctionFactory::Instance().createInitialized(function->asString());

  IAlgorithm_sptr fit = AlgorithmManager::Instance().create("Fit");
  fit->setAlwaysStoreInADS(false);
  fit->setProperty("Function", funcToFit);
  fit->setProperty("InputWorkspace", dataToFit);
  fit->setProperty("CreateOutput", true);

  // Run async so that progress can be shown
  Poco::ActiveResult<bool> result(fit->executeAsync());
  while (!result.available()) {
    QCoreApplication::processEvents();
  }
  if (!result.error().empty()) {
    throw std::runtime_error(result.error());
  }

  MatrixWorkspace_sptr fitOutput = fit->getProperty("OutputWorkspace");
  m_parameterTable = fit->getProperty("OutputParameters");

  enableDisabledPoints(fitOutput, m_data);
  setErrorsAfterFit(fitOutput);

  setCorrectedData(fitOutput);
  setFittedFunction(funcToFit);
  m_sections = sections;
}

void ALCBaselineModellingModel::setData(MatrixWorkspace_sptr data) { m_data = std::move(data); }

/**
 * Disable points in the workpsace in the way that points which are not included
 * in any of specified
 * sections are not used when fitting given workspace
 * @param ws :: Workspace to disable points in
 * @param sections :: Section we want to use for fitting
 */
void ALCBaselineModellingModel::disableUnwantedPoints(
    const MatrixWorkspace_sptr &ws, const std::vector<IALCBaselineModellingModel::Section> &sections) {
  // Whether point with particular index should be disabled
  const size_t numBins = ws->blocksize();
  std::vector<bool> toDisable(numBins, true);

  // Find points which are in at least one section, and exclude them from
  // disable list
  for (size_t i = 0; i < numBins; ++i) {
    const auto it = std::find_if(sections.cbegin(), sections.cend(), [&ws, i](const auto &section) {
      return ws->x(0)[i] >= section.first && ws->x(0)[i] <= section.second;
    });
    if (it != sections.cend()) {
      toDisable[i] = false;
    }
  }

  // XXX: Points are disabled by settings their errors to very high value. This
  // makes those
  //      points to have very low weights during the fitting, effectively
  //      disabling them.

  const double DISABLED_ERR = std::numeric_limits<double>::max();

  // Disable chosen points
  for (size_t i = 0; i < numBins; ++i) {
    if (toDisable[i]) {
      ws->mutableE(0)[i] = DISABLED_ERR;
    }
  }
}

/**
 * Enable points that were disabled for fit
 * @param destWs :: Workspace to enable points in
 * @param sourceWs :: Workspace with original errors
 */
void ALCBaselineModellingModel::enableDisabledPoints(const MatrixWorkspace_sptr &destWs,
                                                     const MatrixWorkspace_const_sptr &sourceWs) {
  // Unwanted points were disabled by setting their errors to very high values.
  // We recover here the original errors stored in sourceWs
  destWs->mutableE(0) = sourceWs->e(0);
}

/**
 * Set errors in Diff spectrum after a fit
 * @param data :: [input/output] Workspace containing spectrum to set errors to
 */
void ALCBaselineModellingModel::setErrorsAfterFit(const MatrixWorkspace_sptr &data) { data->mutableE(2) = data->e(0); }

MatrixWorkspace_sptr ALCBaselineModellingModel::exportWorkspace() const {
  if (m_data) {
    return std::const_pointer_cast<MatrixWorkspace>(m_data);
  }
  return nullptr;
}

ITableWorkspace_sptr ALCBaselineModellingModel::exportSections() const {
  if (!m_sections.empty()) {

    ITableWorkspace_sptr table = WorkspaceFactory::Instance().createTable("TableWorkspace");

    table->addColumn("double", "Start X");
    table->addColumn("double", "End X");

    for (auto const &section : m_sections) {
      TableRow newRow = table->appendRow();
      newRow << section.first << section.second;
    }

    return table;

  } else {

    return ITableWorkspace_sptr();
  }
}

ITableWorkspace_sptr ALCBaselineModellingModel::exportModel() const {
  if (m_parameterTable) {

    return m_parameterTable;

  } else {

    return ITableWorkspace_sptr();
  }
}

void ALCBaselineModellingModel::setCorrectedData(MatrixWorkspace_sptr data) { m_data = std::move(data); }

void ALCBaselineModellingModel::setFittedFunction(IFunction_const_sptr function) {
  m_fittedFunction = std::move(function);
}

MatrixWorkspace_sptr ALCBaselineModellingModel::data() const {
  if (m_data) {
    return extractSpectrum(std::const_pointer_cast<MatrixWorkspace>(m_data), 0);
  }
  return MatrixWorkspace_sptr();
}

MatrixWorkspace_sptr ALCBaselineModellingModel::correctedData() const {
  if (m_data && (m_data->getNumberHistograms() == 3)) {
    return extractSpectrum(std::const_pointer_cast<MatrixWorkspace>(m_data), 2);
  }
  return MatrixWorkspace_sptr();
}

MatrixWorkspace_sptr ALCBaselineModellingModel::baselineData(IFunction_const_sptr function,
                                                             const std::vector<double> &xValues) const {
  const auto inputWorkspace = std::dynamic_pointer_cast<MatrixWorkspace>(
      WorkspaceFactory::Instance().create("Workspace2D", 1, xValues.size(), xValues.size()));

  inputWorkspace->mutableX(0) = xValues;
  return extractSpectrum(evaluateFunction(function, inputWorkspace), 1);
}

} // namespace MantidQt::CustomInterfaces
