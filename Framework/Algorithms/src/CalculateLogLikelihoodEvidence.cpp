// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CalculateLogLikelihoodEvidence.h"

#include "MantidAPI/ADSValidator.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/ArrayProperty.h"

#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceProperty.h"

namespace Mantid::Algorithms {

DECLARE_ALGORITHM(CalculateLogLikelihoodEvidence)

using namespace API;

std::string const DEFAULT_REL_FACTORS_GROUP_NAME = "RelativeFactors";
std::string const DEFAULT_LOG_EVIDENCE_TABLE_NAME = "LogLikelihoodEvidence";

void CalculateLogLikelihoodEvidence::init() {
  declareProperty(
      std::make_unique<Kernel::ArrayProperty<std::string>>("WorkspaceList", std::make_shared<ADSValidator>(true, true)),
      "Names of input Chi2 workspaces to calculate log likelihood evidence and compare relative factors between them.");

  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>(
                      "OutputWorkspace", DEFAULT_LOG_EVIDENCE_TABLE_NAME, Kernel::Direction::Output),
                  "Output table containing log-likelihood evidence values for each input workspace.");

  declareProperty(std::make_unique<WorkspaceProperty<WorkspaceGroup>>(
                      "OutputRelativeFactors", DEFAULT_REL_FACTORS_GROUP_NAME, Kernel::Direction::Output),
                  "Output workspace group containing tables for relative factors between input workspaces.");
}

void CalculateLogLikelihoodEvidence::exec() {
  const std::vector<std::string> workspaceNames = getProperty("WorkspaceList");

  auto logEvidenceTable = std::make_shared<DataObjects::TableWorkspace>();
  logEvidenceTable->addColumn("str", "Workspace");
  logEvidenceTable->addColumn("double", "LogLikelihoodEvidence");

  std::map<std::string, double> wsNameToLogEvidence;
  for (auto &wsName : workspaceNames) {
    if (!AnalysisDataService::Instance().doesExist(wsName)) {
      continue;
    }
    auto chi2Workspace = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);
    if (!chi2Workspace) {
      continue;
    }
    wsNameToLogEvidence[wsName] = calculateLogLikelihoodEvidence(chi2Workspace);
    API::TableRow row = logEvidenceTable->appendRow();
    row << wsName << wsNameToLogEvidence[wsName];
  }
  AnalysisDataService::Instance().addOrReplace(getProperty("OutputWorkspace"), logEvidenceTable);

  auto groupPdf = std::make_shared<WorkspaceGroup>();
  AnalysisDataService::Instance().addOrReplace(getPropertyValue("OutputRelativeFactors"), groupPdf);
  for (const auto &[wsName, evidence] : wsNameToLogEvidence) {
    // Output table for relative log evidence values for each workspace
    auto table = std::make_shared<DataObjects::TableWorkspace>();
    AnalysisDataService::Instance().addOrReplace(wsName + "_RelativeFactors", table);
    groupPdf->addWorkspace(table);
    table->addColumn("str", "Workspaces Relative Log Evidence");
    table->addColumn("double", "Relative Log Evidence Values");
    table->addColumn("str", "Workspaces Bayes Factor");
    table->addColumn("double", "Bayes Factor");

    // Fill in rows with relative factors
    for (const auto &[wsNameSecond, evidenceSecond] : wsNameToLogEvidence) {
      if (wsName == wsNameSecond) {
        continue;
      }
      API::TableRow row = table->appendRow();
      row << wsName + " - " + wsNameSecond << evidence - evidenceSecond << wsName + " / " + wsNameSecond
          << std::exp(evidence - evidenceSecond);
    }
  }
  setProperty("OutputWorkspace", logEvidenceTable);
  setProperty("OutputRelativeFactors", groupPdf);
}

double CalculateLogLikelihoodEvidence::calculateLogLikelihoodEvidence(const MatrixWorkspace_sptr chi2Workspace) const {
  if (!chi2Workspace || chi2Workspace->getNumberHistograms() == 0) {
    return 0.0;
  }

  // Workspace should contain only one histogram with chi2 probability profile
  const MantidVec &chi2 = chi2Workspace->readX(0);
  const MantidVec &probChi2 = chi2Workspace->readY(0);

  if (chi2.size() < 2 || probChi2.empty()) {
    return 0.0;
  }

  // log ∫ exp(-chi2/2) P(chi2) dchi2
  // Use log-sum-exp trick for numerical stability, otherwise the integral can underflow to zero.

  const auto nBins = std::min(probChi2.size(), chi2.size() - 1);
  double maxLogTerm = -std::numeric_limits<double>::infinity();

  // First pass: find the maximum finite log-term for numerical stability.
  for (size_t i = 0; i < nBins; ++i) {
    const double dchi2 = chi2[i + 1] - chi2[i];
    if (probChi2[i] <= 0.0 || dchi2 <= 0.0) {
      continue;
    }
    const double chi2Value = (chi2[i] + chi2[i + 1]) / 2;
    const double logTerm = std::log(probChi2[i]) - 0.5 * chi2Value + std::log(dchi2);
    if (logTerm > maxLogTerm) {
      maxLogTerm = logTerm;
    }
  }

  if (!std::isfinite(maxLogTerm)) {
    return -std::numeric_limits<double>::infinity();
  }

  // Second pass: log-sum-exp of all valid terms.
  double sumExp = 0.0;
  for (size_t i = 0; i < nBins; ++i) {
    const double dchi2 = chi2[i + 1] - chi2[i];
    if (probChi2[i] <= 0.0 || dchi2 <= 0.0) {
      continue;
    }
    const double chi2Value = (chi2[i] + chi2[i + 1]) / 2;
    const double logTerm = std::log(probChi2[i]) - 0.5 * chi2Value + std::log(dchi2);
    sumExp += std::exp(logTerm - maxLogTerm);
  }

  if (sumExp <= 0.0) {
    return -std::numeric_limits<double>::infinity();
  }

  // log(sum_i exp(a_i)) = m + log(sum_i exp(a_i - m)), with m = max_i(a_i).
  // Here a_i = log(P_i) - chi2_i/2 + log(dchi2_i).
  return maxLogTerm + std::log(sumExp);
}

} // namespace Mantid::Algorithms
