#include "EnggDiffGSASFittingModel.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"

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

boost::optional<size_t> getBankID(API::MatrixWorkspace_const_sptr ws) {
  const static std::string bankIDPropertyName = "bankid";
  if (ws->run().hasProperty(bankIDPropertyName)) {
    const auto log = dynamic_cast<Kernel::PropertyWithValue<int> *>(
        ws->run().getLogData(bankIDPropertyName));
    return boost::lexical_cast<size_t>(log->value());
  }
  return boost::none;
}

} // anonymous namespace

namespace MantidQt {
namespace CustomInterfaces {
void EnggDiffGSASFittingModel::addFitResultsToMaps(
    const RunLabel &runLabel, const double rwp,
    const std::string &fittedPeaksWSName,
    const std::string &latticeParamsTableName) {
  addRwp(runLabel, rwp);

  API::AnalysisDataServiceImpl &ADS = API::AnalysisDataService::Instance();
  const auto fittedPeaks =
      ADS.retrieveWS<API::MatrixWorkspace>(fittedPeaksWSName);
  addFittedPeaks(runLabel, fittedPeaks);

  const auto latticeParams =
      ADS.retrieveWS<API::ITableWorkspace>(latticeParamsTableName);
  addLatticeParams(runLabel, latticeParams);
}

void EnggDiffGSASFittingModel::addFittedPeaks(const RunLabel &runLabel,
                                              API::MatrixWorkspace_sptr ws) {
  m_fittedPeaksMap.add(runLabel, ws);
}

void EnggDiffGSASFittingModel::addFocusedRun(const RunLabel &runLabel,
                                             API::MatrixWorkspace_sptr ws) {
  m_focusedWorkspaceMap.add(runLabel, ws);
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

boost::optional<std::string> EnggDiffGSASFittingModel::doPawleyRefinement(
    const RunLabel &runLabel, const std::string &instParamFile,
    const std::vector<std::string> &phaseFiles, const std::string &pathToGSASII,
    const std::string &GSASIIProjectFile, const double dMin,
    const double negativeWeight) {
  const auto inputWS = getFocusedWorkspace(runLabel);
  if (!inputWS) {
    return "Could not find focused run for run number " +
           std::to_string(runLabel.runNumber) + " and bank ID " +
           std::to_string(runLabel.bank);
  }
  const auto outputWSName = generateFittedPeaksWSName(runLabel);
  const auto latticeParamsName = generateLatticeParamsName(runLabel);

  double rwp = 0;

  try {
    rwp = doGSASRefinementAlgorithm(*inputWS, outputWSName, latticeParamsName,
                                    "Pawley refinement", instParamFile,
                                    phaseFiles, pathToGSASII, GSASIIProjectFile,
                                    dMin, negativeWeight);
  } catch (const std::exception &e) {
    return boost::make_optional<std::string>(e.what());
  }

  addFitResultsToMaps(runLabel, rwp, outputWSName, latticeParamsName);

  return boost::none;
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
  gsasAlg->setProperty("PhaseInfoFiles", phaseFiles);
  gsasAlg->setProperty("PathToGSASII", pathToGSASII);
  gsasAlg->setProperty("SaveGSASIIProjectFile", GSASIIProjectFile);
  gsasAlg->setProperty("PawleyDMin", dMin);
  gsasAlg->setProperty("PawleyNegativeWeight", negativeWeight);
  gsasAlg->execute();

  const double rwp = gsasAlg->getProperty("Rwp");
  return rwp;
}

boost::optional<std::string> EnggDiffGSASFittingModel::doRietveldRefinement(
    const RunLabel &runLabel, const std::string &instParamFile,
    const std::vector<std::string> &phaseFiles, const std::string &pathToGSASII,
    const std::string &GSASIIProjectFile) {
  const auto inputWS = getFocusedWorkspace(runLabel);
  if (!inputWS) {
    return "Could not find focused run for run number " +
           std::to_string(runLabel.runNumber) + " and bank ID " +
           std::to_string(runLabel.bank);
  }

  const auto outputWSName = generateFittedPeaksWSName(runLabel);
  const auto latticeParamsName = generateLatticeParamsName(runLabel);

  double rwp = 0;
  try {
    rwp = doGSASRefinementAlgorithm(
        *inputWS, outputWSName, latticeParamsName, "Rietveld refinement",
        instParamFile, phaseFiles, pathToGSASII, GSASIIProjectFile,
        DEFAULT_PAWLEY_DMIN, DEFAULT_PAWLEY_NEGATIVE_WEIGHT);
  }

  catch (const std::exception &e) {
    return boost::make_optional<std::string>(e.what());
  }
  addFitResultsToMaps(runLabel, rwp, outputWSName, latticeParamsName);

  return boost::none;
}

boost::optional<API::MatrixWorkspace_sptr>
EnggDiffGSASFittingModel::getFittedPeaks(const RunLabel &runLabel) const {
  return getFromRunMapOptional(m_fittedPeaksMap, runLabel);
}

boost::optional<API::MatrixWorkspace_sptr>
EnggDiffGSASFittingModel::getFocusedWorkspace(const RunLabel &runLabel) const {
  return getFromRunMapOptional(m_focusedWorkspaceMap, runLabel);
}

boost::optional<API::ITableWorkspace_sptr>
EnggDiffGSASFittingModel::getLatticeParams(const RunLabel &runLabel) const {
  return getFromRunMapOptional(m_latticeParamsMap, runLabel);
}

std::vector<RunLabel> EnggDiffGSASFittingModel::getRunLabels() const {
  return m_focusedWorkspaceMap.getRunLabels();
}

boost::optional<double>
EnggDiffGSASFittingModel::getRwp(const RunLabel &runLabel) const {
  return getFromRunMapOptional(m_rwpMap, runLabel);
}

bool EnggDiffGSASFittingModel::hasFittedPeaksForRun(
    const RunLabel &runLabel) const {
  return m_fittedPeaksMap.contains(runLabel);
}

bool EnggDiffGSASFittingModel::hasFocusedRun(const RunLabel &runLabel) const {
  return m_focusedWorkspaceMap.contains(runLabel);
}

boost::optional<std::string>
EnggDiffGSASFittingModel::loadFocusedRun(const std::string &filename) {
  const auto wsName = stripWSNameFromFilename(filename);

  try {
    auto loadAlg = API::AlgorithmManager::Instance().create("Load");
    loadAlg->setProperty("Filename", filename);
    loadAlg->setProperty("OutputWorkspace", wsName);
    loadAlg->execute();
  } catch (const std::exception &e) {
    return boost::make_optional<std::string>(e.what());
  }

  API::AnalysisDataServiceImpl &ADS = API::AnalysisDataService::Instance();
  const auto ws = ADS.retrieveWS<API::MatrixWorkspace>(wsName);

  const auto runNumber = ws->getRunNumber();
  const auto bankID = getBankID(ws);

  if (!bankID) {
    return "Bank ID was not set in the sample logs for run " +
           std::to_string(runNumber) +
           ". Make sure it has been focused correctly";
  }

  m_focusedWorkspaceMap.add(RunLabel(runNumber, *bankID), ws);
  return boost::none;
}

} // CustomInterfaces
} // MantidQt
