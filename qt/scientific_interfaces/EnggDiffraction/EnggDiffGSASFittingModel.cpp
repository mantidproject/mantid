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
    const API::MatrixWorkspace_sptr inputWS, const RunLabel &runLabel,
    const std::string &instParamFile,
    const std::vector<std::string> &phaseFiles, const std::string &pathToGSASII,
    const std::string &GSASIIProjectFile, const double dMin,
    const double negativeWeight) {
  const auto outputWSName = generateFittedPeaksWSName(runLabel);
  const auto latticeParamsName = generateLatticeParamsName(runLabel);

  const auto rwp = doGSASRefinementAlgorithm(
      inputWS, outputWSName, latticeParamsName, "Pawley refinement",
      instParamFile, phaseFiles, pathToGSASII, GSASIIProjectFile, dMin,
      negativeWeight);

  addFitResultsToMaps(runLabel, rwp, latticeParamsName);

  API::AnalysisDataServiceImpl &ADS = API::AnalysisDataService::Instance();
  const auto fittedPeaks = ADS.retrieveWS<API::MatrixWorkspace>(outputWSName);

  return fittedPeaks;
}

double EnggDiffGSASFittingModel::doGSASRefinementAlgorithm(
    API::MatrixWorkspace_sptr inputWorkspace,
    const std::string &outputWorkspaceName,
    const std::string &latticeParamsName, const std::string &refinementMethod,
    const std::string &instParamFile,
    const std::vector<std::string> &phaseFiles, const std::string &pathToGSASII,
    const std::string &GSASIIProjectFile, const double dMin,
    const double negativeWeight) {
  auto gsasAlg =
      API::AlgorithmManager::Instance().create("GSASIIRefineFitPeaks");
  gsasAlg->setProperty("RefinementMethod", refinementMethod);
  gsasAlg->setProperty("InputWorkspace", inputWorkspace);
  gsasAlg->setProperty("OutputWorkspace", outputWorkspaceName);
  gsasAlg->setProperty("LatticeParameters", latticeParamsName);
  gsasAlg->setProperty("InstrumentFile", instParamFile);
  gsasAlg->setProperty("PhaseInfoFiles", boost::algorithm::join(phaseFiles, ","));
  gsasAlg->setProperty("PathToGSASII", pathToGSASII);
  gsasAlg->setProperty("SaveGSASIIProjectFile", GSASIIProjectFile);
  gsasAlg->setProperty("PawleyDMin", dMin);
  gsasAlg->setProperty("PawleyNegativeWeight", negativeWeight);
  gsasAlg->execute();

  const double rwp = gsasAlg->getProperty("Rwp");
  return rwp;
}

API::MatrixWorkspace_sptr EnggDiffGSASFittingModel::doRietveldRefinement(
    const API::MatrixWorkspace_sptr inputWS, const RunLabel &runLabel,
    const std::string &instParamFile,
    const std::vector<std::string> &phaseFiles, const std::string &pathToGSASII,
    const std::string &GSASIIProjectFile) {
  const auto outputWSName = generateFittedPeaksWSName(runLabel);
  const auto latticeParamsName = generateLatticeParamsName(runLabel);

  const auto rwp = doGSASRefinementAlgorithm(
      inputWS, outputWSName, latticeParamsName, "Rietveld refinement",
      instParamFile, phaseFiles, pathToGSASII, GSASIIProjectFile,
      DEFAULT_PAWLEY_DMIN, DEFAULT_PAWLEY_NEGATIVE_WEIGHT);

  addFitResultsToMaps(runLabel, rwp, latticeParamsName);

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
