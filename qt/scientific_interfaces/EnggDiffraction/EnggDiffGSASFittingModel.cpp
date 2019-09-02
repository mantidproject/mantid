// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "EnggDiffGSASFittingModel.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Common/MantidAlgorithmMetatype.h"

#include <boost/algorithm/string/join.hpp>

using namespace Mantid;

namespace {

std::string stripWSNameFromFilename(const std::string &fullyQualifiedFilename) {
  std::vector<std::string> directories;
  boost::split(directories, fullyQualifiedFilename, boost::is_any_of("\\/"));
  const std::string filename = directories.back();
  std::vector<std::string> filenameSegments;
  boost::split(filenameSegments, filename, boost::is_any_of("."));
  return filenameSegments[0];
}

std::string refinementMethodToString(
    const MantidQt::CustomInterfaces::GSASRefinementMethod &method) {
  switch (method) {
  case MantidQt::CustomInterfaces::GSASRefinementMethod::PAWLEY:
    return "Pawley refinement";
  case MantidQt::CustomInterfaces::GSASRefinementMethod::RIETVELD:
    return "Rietveld refinement";
  default:
    throw std::invalid_argument(
        "Invalid refinement method: please contact the development team");
  }
}

} // anonymous namespace

namespace MantidQt {
namespace CustomInterfaces {

EnggDiffGSASFittingModel::EnggDiffGSASFittingModel() {
  qRegisterMetaType<
      MantidQt::CustomInterfaces::GSASIIRefineFitPeaksOutputProperties>(
      "GSASIIRefineFitPeaksOutputProperties");
  qRegisterMetaType<Mantid::API::IAlgorithm_sptr>("IAlgorithm_sptr");
  qRegisterMetaType<std::vector<GSASIIRefineFitPeaksOutputProperties>>(
      "std::vector<GSASIIRefineFitPeaksOutputProperties>");
}

EnggDiffGSASFittingModel::~EnggDiffGSASFittingModel() {
  if (m_workerThread) {
    if (m_workerThread->isRunning()) {
      m_workerThread->wait(10);
    }
  }
}

void EnggDiffGSASFittingModel::addFitResultsToMaps(
    const RunLabel &runLabel, const double rwp, const double sigma,
    const double gamma, const API::ITableWorkspace_sptr latticeParams) {
  addRwp(runLabel, rwp);
  addSigma(runLabel, sigma);
  addGamma(runLabel, gamma);
  addLatticeParams(runLabel, latticeParams);
}

void EnggDiffGSASFittingModel::addLatticeParams(
    const RunLabel &runLabel, API::ITableWorkspace_sptr table) {
  m_latticeParamsMap.add(runLabel, table);
}

void EnggDiffGSASFittingModel::addGamma(const RunLabel &runLabel,
                                        const double gamma) {
  m_gammaMap.add(runLabel, gamma);
}

void EnggDiffGSASFittingModel::addRwp(const RunLabel &runLabel,
                                      const double rwp) {
  m_rwpMap.add(runLabel, rwp);
}

void EnggDiffGSASFittingModel::addSigma(const RunLabel &runLabel,
                                        const double sigma) {
  m_sigmaMap.add(runLabel, sigma);
}

namespace {

std::string generateFittedPeaksWSName(const RunLabel &runLabel) {
  return runLabel.runNumber + "_" + std::to_string(runLabel.bank) +
         "_gsasii_fitted_peaks";
}

std::string generateLatticeParamsName(const RunLabel &runLabel) {
  return runLabel.runNumber + "_" + std::to_string(runLabel.bank) +
         "_lattice_params";
}
} // namespace

std::pair<API::IAlgorithm_sptr, GSASIIRefineFitPeaksOutputProperties>
EnggDiffGSASFittingModel::doGSASRefinementAlgorithm(
    const GSASIIRefineFitPeaksParameters &params) {
  auto gsasAlg =
      API::AlgorithmManager::Instance().create("GSASIIRefineFitPeaks");

  gsasAlg->setProperty("RefinementMethod",
                       refinementMethodToString(params.refinementMethod));
  gsasAlg->setProperty("InputWorkspace", params.inputWorkspace);
  gsasAlg->setProperty("InstrumentFile", params.instParamsFile);
  gsasAlg->setProperty("PhaseInfoFiles",
                       boost::algorithm::join(params.phaseFiles, ","));
  gsasAlg->setProperty("PathToGSASII", params.gsasHome);

  if (params.dMin) {
    gsasAlg->setProperty("PawleyDMin", *(params.dMin));
  }
  if (params.negativeWeight) {
    gsasAlg->setProperty("PawleyNegativeWeight", *(params.negativeWeight));
  }
  if (params.xMin) {
    gsasAlg->setProperty("XMin", *(params.xMin));
  }
  if (params.xMax) {
    gsasAlg->setProperty("XMax", *(params.xMax));
  }
  gsasAlg->setProperty("RefineSigma", params.refineSigma);
  gsasAlg->setProperty("RefineGamma", params.refineGamma);

  const auto outputWSName = generateFittedPeaksWSName(params.runLabel);
  const auto latticeParamsName = generateLatticeParamsName(params.runLabel);
  gsasAlg->setProperty("OutputWorkspace", outputWSName);
  gsasAlg->setProperty("LatticeParameters", latticeParamsName);
  gsasAlg->setProperty("SaveGSASIIProjectFile", params.gsasProjectFile);
  gsasAlg->execute();

  const double rwp = gsasAlg->getProperty("Rwp");
  const double sigma = gsasAlg->getProperty("Sigma");
  const double gamma = gsasAlg->getProperty("Gamma");

  API::AnalysisDataServiceImpl &ADS = API::AnalysisDataService::Instance();
  const auto fittedPeaks = ADS.retrieveWS<API::MatrixWorkspace>(outputWSName);
  const auto latticeParams =
      ADS.retrieveWS<API::ITableWorkspace>(latticeParamsName);
  return std::make_pair(gsasAlg, GSASIIRefineFitPeaksOutputProperties(
                                     rwp, sigma, gamma, fittedPeaks,
                                     latticeParams, params.runLabel));
}

void EnggDiffGSASFittingModel::doRefinements(
    const std::vector<GSASIIRefineFitPeaksParameters> &params) {
  m_workerThread = std::make_unique<QThread>(this);
  EnggDiffGSASFittingWorker *worker =
      new EnggDiffGSASFittingWorker(this, params);
  worker->moveToThread(m_workerThread.get());

  connect(m_workerThread.get(), SIGNAL(started()), worker,
          SLOT(doRefinements()));
  connect(worker,
          SIGNAL(refinementSuccessful(Mantid::API::IAlgorithm_sptr,
                                      GSASIIRefineFitPeaksOutputProperties)),
          this,
          SLOT(processRefinementSuccessful(
              Mantid::API::IAlgorithm_sptr,
              const GSASIIRefineFitPeaksOutputProperties &)));
  connect(worker,
          SIGNAL(refinementsComplete(
              Mantid::API::IAlgorithm_sptr,
              std::vector<GSASIIRefineFitPeaksOutputProperties>)),
          this,
          SLOT(processRefinementsComplete(
              Mantid::API::IAlgorithm_sptr,
              const std::vector<GSASIIRefineFitPeaksOutputProperties> &)));
  connect(worker, SIGNAL(refinementFailed(const std::string &)), this,
          SLOT(processRefinementFailed(const std::string &)));
  connect(worker, SIGNAL(refinementCancelled()), this,
          SLOT(processRefinementCancelled()));
  connect(m_workerThread.get(), SIGNAL(finished()), m_workerThread.get(),
          SLOT(deleteLater()));
  connect(worker,
          SIGNAL(refinementSuccessful(Mantid::API::IAlgorithm_sptr,
                                      GSASIIRefineFitPeaksOutputProperties)),
          worker, SLOT(deleteLater()));
  connect(worker, SIGNAL(refinementFailed(const std::string &)), worker,
          SLOT(deleteLater()));
  m_workerThread->start();
}

boost::optional<API::ITableWorkspace_sptr>
EnggDiffGSASFittingModel::getLatticeParams(const RunLabel &runLabel) const {
  return getFromRunMapOptional(m_latticeParamsMap, runLabel);
}

boost::optional<double>
EnggDiffGSASFittingModel::getGamma(const RunLabel &runLabel) const {
  return getFromRunMapOptional(m_gammaMap, runLabel);
}

boost::optional<double>
EnggDiffGSASFittingModel::getRwp(const RunLabel &runLabel) const {
  return getFromRunMapOptional(m_rwpMap, runLabel);
}

boost::optional<double>
EnggDiffGSASFittingModel::getSigma(const RunLabel &runLabel) const {
  return getFromRunMapOptional(m_sigmaMap, runLabel);
}

bool EnggDiffGSASFittingModel::hasFitResultsForRun(
    const RunLabel &runLabel) const {
  return m_rwpMap.contains(runLabel) && m_sigmaMap.contains(runLabel) &&
         m_gammaMap.contains(runLabel);
}

Mantid::API::MatrixWorkspace_sptr
EnggDiffGSASFittingModel::loadFocusedRun(const std::string &filename) const {
  const auto wsName = stripWSNameFromFilename(filename);

  auto loadAlg = API::AlgorithmManager::Instance().create("Load");
  loadAlg->setProperty("Filename", filename);
  loadAlg->setProperty("OutputWorkspace", wsName);
  loadAlg->execute();

  API::AnalysisDataServiceImpl &ADS = API::AnalysisDataService::Instance();
  auto wsTest = ADS.retrieveWS<API::Workspace>(wsName);
  const auto ws = boost::dynamic_pointer_cast<API::MatrixWorkspace>(wsTest);
  if (!ws) {
    throw std::invalid_argument(
        "Invalid Workspace loaded, are you sure it has been focused?");
  }
  return ws;
}

void EnggDiffGSASFittingModel::processRefinementsComplete(
    Mantid::API::IAlgorithm_sptr alg,
    const std::vector<GSASIIRefineFitPeaksOutputProperties>
        &refinementResultSets) {
  m_observer->notifyRefinementsComplete(alg, refinementResultSets);
}

void EnggDiffGSASFittingModel::processRefinementFailed(
    const std::string &failureMessage) {
  if (m_observer) {
    m_observer->notifyRefinementFailed(failureMessage);
  }
}

void EnggDiffGSASFittingModel::processRefinementSuccessful(
    API::IAlgorithm_sptr successfulAlgorithm,
    const GSASIIRefineFitPeaksOutputProperties &refinementResults) {
  addFitResultsToMaps(refinementResults.runLabel, refinementResults.rwp,
                      refinementResults.sigma, refinementResults.gamma,
                      refinementResults.latticeParamsWS);
  if (m_observer) {
    m_observer->notifyRefinementSuccessful(successfulAlgorithm,
                                           refinementResults);
  }
}

void EnggDiffGSASFittingModel::processRefinementCancelled() {
  if (m_observer) {
    m_observer->notifyRefinementCancelled();
  }
}

void EnggDiffGSASFittingModel::saveRefinementResultsToHDF5(
    const Mantid::API::IAlgorithm_sptr successfulAlg,
    const std::vector<GSASIIRefineFitPeaksOutputProperties>
        &refinementResultSets,
    const std::string &filename) const {
  auto saveAlg = API::AlgorithmManager::Instance().create(
      "EnggSaveGSASIIFitResultsToHDF5");

  const auto numRuns = refinementResultSets.size();
  std::vector<std::string> latticeParamWSNames;
  latticeParamWSNames.reserve(numRuns);
  std::vector<std::string> runNumbers;
  runNumbers.reserve(numRuns);
  std::vector<long> bankIDs;
  bankIDs.reserve(numRuns);
  std::vector<double> sigmas;
  sigmas.reserve(numRuns);
  std::vector<double> gammas;
  gammas.reserve(numRuns);
  std::vector<double> rwps;
  rwps.reserve(numRuns);

  const bool refineSigma = successfulAlg->getProperty("RefineSigma");
  saveAlg->setProperty("RefineSigma", refineSigma);
  const bool refineGamma = successfulAlg->getProperty("RefineGamma");
  saveAlg->setProperty("RefineGamma", refineGamma);

  for (const auto &refinementResults : refinementResultSets) {
    const auto &runLabel = refinementResults.runLabel;
    const auto latticeParams = *getLatticeParams(runLabel);

    latticeParamWSNames.emplace_back(latticeParams->getName());
    runNumbers.emplace_back(runLabel.runNumber);
    bankIDs.emplace_back(static_cast<long>(runLabel.bank));
    rwps.emplace_back(refinementResults.rwp);

    if (refineSigma) {
      sigmas.emplace_back(refinementResults.sigma);
    }
    if (refineGamma) {
      gammas.emplace_back(refinementResults.gamma);
    }
  }

  saveAlg->setProperty("LatticeParamWorkspaces", latticeParamWSNames);
  saveAlg->setProperty("BankIDs", bankIDs);
  saveAlg->setProperty("RunNumbers", runNumbers);

  const std::string refinementMethod =
      successfulAlg->getProperty("RefinementMethod");
  saveAlg->setProperty("RefinementMethod", refinementMethod);
  saveAlg->setProperty("XMin", successfulAlg->getPropertyValue("XMin"));
  saveAlg->setProperty("XMax", successfulAlg->getPropertyValue("XMax"));

  if (refinementMethod == "Pawley refinement") {
    saveAlg->setProperty("PawleyDMin",
                         successfulAlg->getPropertyValue("PawleyDMin"));
    saveAlg->setProperty(
        "PawleyNegativeWeight",
        successfulAlg->getPropertyValue("PawleyNegativeWeight"));
  }

  if (refineSigma) {
    saveAlg->setProperty("Sigma", sigmas);
  }

  if (refineGamma) {
    saveAlg->setProperty("Gamma", gammas);
  }

  saveAlg->setProperty("Rwp", rwps);
  saveAlg->setProperty("Filename", filename);
  saveAlg->execute();
}

void EnggDiffGSASFittingModel::setObserver(
    boost::shared_ptr<IEnggDiffGSASFittingObserver> observer) {
  m_observer = observer;
}

} // namespace CustomInterfaces
} // namespace MantidQt
