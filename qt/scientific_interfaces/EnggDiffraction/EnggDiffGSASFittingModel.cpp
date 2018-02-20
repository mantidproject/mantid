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
    const RunLabel &runLabel, const double rwp,
    const std::string &latticeParamsTableName) {
  addRwp(runLabel, rwp);

  API::AnalysisDataServiceImpl &ADS = API::AnalysisDataService::Instance();
  const auto latticeParams =
      ADS.retrieveWS<API::ITableWorkspace>(latticeParamsTableName);
  addLatticeParams(runLabel, latticeParams);
}

void EnggDiffGSASFittingModel::addLatticeParams(
    const RunLabel &runLabel, API::ITableWorkspace_sptr table) {
  m_latticeParamsMap.add(runLabel, table);
}

void EnggDiffGSASFittingModel::addRwp(const RunLabel &runLabel,
                                      const double rwp) {
  m_rwpMap.add(runLabel, rwp);
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

  const auto rwp = doGSASRefinementAlgorithm(outputWSName, latticeParamsName,
                                             params, "Pawley refinement");

  addFitResultsToMaps(params.runLabel, rwp, latticeParamsName);

  API::AnalysisDataServiceImpl &ADS = API::AnalysisDataService::Instance();
  const auto fittedPeaks = ADS.retrieveWS<API::MatrixWorkspace>(outputWSName);

  return fittedPeaks;
}

double EnggDiffGSASFittingModel::doGSASRefinementAlgorithm(
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
  return rwp;
}

API::MatrixWorkspace_sptr EnggDiffGSASFittingModel::doRietveldRefinement(
    const GSASIIRefineFitPeaksParameters &params) {
  const auto outputWSName = generateFittedPeaksWSName(params.runLabel);
  const auto latticeParamsName = generateLatticeParamsName(params.runLabel);

  const auto rwp = doGSASRefinementAlgorithm(outputWSName, latticeParamsName,
                                             params, "Rietveld refinement");

  addFitResultsToMaps(params.runLabel, rwp, latticeParamsName);

  API::AnalysisDataServiceImpl &ADS = API::AnalysisDataService::Instance();
  const auto fittedPeaks = ADS.retrieveWS<API::MatrixWorkspace>(outputWSName);

  return fittedPeaks;
}

boost::optional<API::ITableWorkspace_sptr>
EnggDiffGSASFittingModel::getLatticeParams(const RunLabel &runLabel) const {
  return getFromRunMapOptional(m_latticeParamsMap, runLabel);
}

boost::optional<double>
EnggDiffGSASFittingModel::getRwp(const RunLabel &runLabel) const {
  return getFromRunMapOptional(m_rwpMap, runLabel);
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
