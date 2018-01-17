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

size_t getBankID(API::MatrixWorkspace_const_sptr ws) {
  const static std::string bankIDPropertyName = "bankid";
  if (ws->run().hasProperty(bankIDPropertyName)) {
    const auto log = dynamic_cast<Kernel::PropertyWithValue<int> *>(
        ws->run().getLogData(bankIDPropertyName));
    return boost::lexical_cast<size_t>(log->value());
  }
  throw std::runtime_error("Bank ID was not set in the sample logs.");
}

} // anonymous namespace

namespace MantidQt {
namespace CustomInterfaces {

void EnggDiffGSASFittingModel::addFittedPeaks(
    const int runNumber, const size_t bank,
    Mantid::API::MatrixWorkspace_sptr ws) {
  m_fittedPeaksMap.add(runNumber, bank, ws);
}

void EnggDiffGSASFittingModel::addFocusedRun(
    const int runNumber, const size_t bank,
    Mantid::API::MatrixWorkspace_sptr ws) {
  m_focusedWorkspaceMap.add(runNumber, bank, ws);
}

bool EnggDiffGSASFittingModel::doPawleyRefinement(
    const int runNumber, const size_t bank, const std::string &instParamFile,
    const std::vector<std::string> &phaseFiles, const std::string &pathToGSASII,
    const std::string &GSASIIProjectFile, const double dMin,
    const double negativeWeight) {
  throw std::runtime_error("Not yet implemented");
}

bool EnggDiffGSASFittingModel::doRietveldRefinement(
    const int runNumber, const size_t bank, const std::string &instParamFile,
    const std::vector<std::string> &phaseFiles, const std::string &pathToGSASII,
    const std::string &GSASIIProjectFile) {
  throw std::runtime_error("Not yet implemented");
}

boost::optional<Mantid::API::MatrixWorkspace_sptr>
EnggDiffGSASFittingModel::getFittedPeaks(const int runNumber,
                                         const size_t bank) const {
  if (m_fittedPeaksMap.contains(runNumber, bank)) {
    return m_fittedPeaksMap.get(runNumber, bank);
  }
  return boost::none;
}

boost::optional<Mantid::API::MatrixWorkspace_sptr>
EnggDiffGSASFittingModel::getFocusedWorkspace(const int runNumber,
                                              const size_t bank) const {
  if (m_focusedWorkspaceMap.contains(runNumber, bank)) {
    return m_focusedWorkspaceMap.get(runNumber, bank);
  }
  return boost::none;
}

boost::optional<Mantid::API::ITableWorkspace_sptr>
EnggDiffGSASFittingModel::getLatticeParams(const int runNumber,
                                           const size_t bank) const {
  throw std::runtime_error("Not yet implemented");
}

std::vector<std::pair<int, size_t>>
EnggDiffGSASFittingModel::getRunLabels() const {
  return m_focusedWorkspaceMap.getRunNumbersAndBankIDs();
}

boost::optional<double>
EnggDiffGSASFittingModel::getRwp(const int runNumber, const size_t bank) const {
  throw std::runtime_error("Not yet implemented");
}

bool EnggDiffGSASFittingModel::hasFittedPeaksForRun(const int runNumber,
                                                    const size_t bank) const {
  return m_fittedPeaksMap.contains(runNumber, bank);
}

bool EnggDiffGSASFittingModel::hasFocusedRun(const int runNumber,
                                             const size_t bank) const {
  return m_focusedWorkspaceMap.contains(runNumber, bank);
}

bool EnggDiffGSASFittingModel::loadFocusedRun(const std::string &filename) {
  const auto wsName = stripWSNameFromFilename(filename);

  try {
    auto loadAlg = API::AlgorithmManager::Instance().create("Load");
    loadAlg->setProperty("Filename", filename);
    loadAlg->setProperty("OutputWorkspace", wsName);
    loadAlg->execute();
  } catch (const std::exception &e) {
    return false;
  }

  API::AnalysisDataServiceImpl &ADS = API::AnalysisDataService::Instance();
  const auto ws = ADS.retrieveWS<API::MatrixWorkspace>(wsName);

  const auto runNumber = ws->getRunNumber();
  const auto bank = getBankID(ws);
  m_focusedWorkspaceMap.add(runNumber, bank, ws);
  return true;
}

} // CustomInterfaces
} // MantidQt
