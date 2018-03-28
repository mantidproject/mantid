#include "EnggDiffGSASFittingModel.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"

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

} // anonymous namespace

namespace MantidQt {
namespace CustomInterfaces {
void EnggDiffGSASFittingModel::addFitResultsToMaps(
    const RunLabel &runLabel, const double rwp, const double sigma,
    const double gamma, const std::string &latticeParamsTableName) {
  addRwp(runLabel, rwp);
  addSigma(runLabel, sigma);
  addGamma(runLabel, gamma);

  API::AnalysisDataServiceImpl &ADS = API::AnalysisDataService::Instance();
  const auto latticeParams =
      ADS.retrieveWS<API::ITableWorkspace>(latticeParamsTableName);
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
  return std::to_string(runLabel.runNumber) + "_" +
         std::to_string(runLabel.bank) + "_gsasii_fitted_peaks";
}

std::string generateLatticeParamsName(const RunLabel &runLabel) {
  return std::to_string(runLabel.runNumber) + "_" +
         std::to_string(runLabel.bank) + "_lattice_params";
}
}

API::MatrixWorkspace_sptr EnggDiffGSASFittingModel::doPawleyRefinement(
    const GSASIIRefineFitPeaksParameters &params) {
  const auto outputWSName = generateFittedPeaksWSName(params.runLabel);
  const auto latticeParamsName = generateLatticeParamsName(params.runLabel);

  const auto outputProperties = doGSASRefinementAlgorithm(
      outputWSName, latticeParamsName, params, "Pawley refinement");

  addFitResultsToMaps(params.runLabel, outputProperties.rwp,
                      outputProperties.sigma, outputProperties.gamma,
                      latticeParamsName);

  API::AnalysisDataServiceImpl &ADS = API::AnalysisDataService::Instance();
  const auto fittedPeaks = ADS.retrieveWS<API::MatrixWorkspace>(outputWSName);

  return fittedPeaks;
}

GSASIIRefineFitPeaksOutputProperties
EnggDiffGSASFittingModel::doGSASRefinementAlgorithm(
    const std::string &outputWorkspaceName,
    const std::string &latticeParamsName,
    const GSASIIRefineFitPeaksParameters &params,
    const std::string &refinementMethod) {
  auto gsasAlg =
      API::AlgorithmManager::Instance().create("GSASIIRefineFitPeaks");

  gsasAlg->setProperty("RefinementMethod", refinementMethod);
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

  gsasAlg->setProperty("OutputWorkspace", outputWorkspaceName);
  gsasAlg->setProperty("LatticeParameters", latticeParamsName);
  gsasAlg->setProperty("SaveGSASIIProjectFile", params.gsasProjectFile);
  gsasAlg->execute();

  const double rwp = gsasAlg->getProperty("Rwp");
  const double sigma = gsasAlg->getProperty("Sigma");
  const double gamma = gsasAlg->getProperty("Gamma");
  return GSASIIRefineFitPeaksOutputProperties(rwp, sigma, gamma);
}

API::MatrixWorkspace_sptr EnggDiffGSASFittingModel::doRietveldRefinement(
    const GSASIIRefineFitPeaksParameters &params) {
  const auto outputWSName = generateFittedPeaksWSName(params.runLabel);
  const auto latticeParamsName = generateLatticeParamsName(params.runLabel);

  const auto outputProperties = doGSASRefinementAlgorithm(
      outputWSName, latticeParamsName, params, "Rietveld refinement");

  addFitResultsToMaps(params.runLabel, outputProperties.rwp,
                      outputProperties.sigma, outputProperties.gamma,
                      latticeParamsName);

  API::AnalysisDataServiceImpl &ADS = API::AnalysisDataService::Instance();
  const auto fittedPeaks = ADS.retrieveWS<API::MatrixWorkspace>(outputWSName);

  return fittedPeaks;
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
  const auto ws = ADS.retrieveWS<API::MatrixWorkspace>(wsName);
  return ws;
}

} // CustomInterfaces
} // MantidQt
