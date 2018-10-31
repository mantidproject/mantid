#include "MantidMuon/MuonPairingAsymmetry.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidMuon/MuonAlgorithmHelper.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

namespace Mantid {
namespace Muon {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MuonPairingAsymmetry)

void MuonPairingAsymmetry::init() {
  std::string emptyString("");

  declareProperty(
      Mantid::Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "OutputWorkspace", emptyString, Direction::Output),
      "The workspace which will hold the results of the asymmetry "
      "calculation.");

  declareProperty("PairName", emptyString,
                  "The name of the pair. Must "
                  "contain at least one alphanumeric "
                  "character.",
                  Direction::Input);

  declareProperty(
      "Alpha", 1.0, boost::make_shared<MandatoryValidator<double>>(),
      "Alpha parameter used in the asymmetry calculation.", Direction::Input);

  declareProperty("SpecifyGroupsManually", false,
                  "Specify the pair of groups manually using the raw data and "
                  "various optional parameters.");

  // Select groups via workspaces

  declareProperty(
      Mantid::Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "InputWorkspace1", emptyString, Direction::Input,
          PropertyMode::Optional),
      "Input workspace containing data from grouped detectors.");

  declareProperty(
      Mantid::Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "InputWorkspace2", emptyString, Direction::Input,
          PropertyMode::Optional),
      "Input workspace containing data from grouped detectors.");

  setPropertySettings("InputWorkspace1",
                      make_unique<Kernel::EnabledWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_EQUAL_TO, "0"));
  setPropertySettings("InputWorkspace2",
                      make_unique<Kernel::EnabledWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_EQUAL_TO, "0"));

  // Specify groups manually

  declareProperty(
      Mantid::Kernel::make_unique<WorkspaceProperty<WorkspaceGroup>>(
          "InputWorkspace", emptyString, Direction::Input,
          PropertyMode::Optional),
      "Input workspace containing data from detectors which are to "
      "be grouped.");
  setPropertySettings("InputWorkspace",
                      make_unique<Kernel::EnabledWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_EQUAL_TO, "1"));

  declareProperty("Group1", std::to_string(1),
                  "The grouping of detectors, comma separated list of detector "
                  "IDs or hyphenated ranges of IDs.",
                  Direction::Input);
  declareProperty("Group2", std::to_string(1),
                  "The grouping of detectors, comma separated list of detector "
                  "IDs or hyphenated ranges of IDs.",
                  Direction::Input);
  setPropertySettings("Group1",
                      make_unique<Kernel::EnabledWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_EQUAL_TO, "1"));
  setPropertySettings("Group2",
                      make_unique<Kernel::EnabledWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_EQUAL_TO, "1"));

  declareProperty(make_unique<ArrayProperty<int>>("SummedPeriods", "1"),
                  "A list of periods to sum in multiperiod data.");
  setPropertySettings("SummedPeriods",
                      make_unique<Kernel::EnabledWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_EQUAL_TO, "1"));

  declareProperty(
      make_unique<ArrayProperty<int>>("SubtractedPeriods", Direction::Input),
      "A list of periods to subtract in multiperiod data.");
  setPropertySettings("SubtractedPeriods",
                      make_unique<Kernel::EnabledWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_EQUAL_TO, "1"));

  // Perform Group Associations.

  std::string workspaceGrp("Specify Group Workspaces");
  setPropertyGroup("InputWorkspace1", workspaceGrp);
  setPropertyGroup("InputWorkspace2", workspaceGrp);

  std::string manualGroupGrp("Specify Detector ID Groups Manually");
  setPropertyGroup("InputWorkspace", manualGroupGrp);
  setPropertyGroup("Group1", manualGroupGrp);
  setPropertyGroup("Group2", manualGroupGrp);

  std::string periodGrp("Multi-period Data");
  setPropertyGroup("SummedPeriods", periodGrp);
  setPropertyGroup("SubtractedPeriods", periodGrp);
}
void MuonPairingAsymmetry::exec() {

  MatrixWorkspace_sptr outWS;

  if (getProperty("SpecifyGroupsManually")) {
    WorkspaceGroup_sptr inputWS = getProperty("InputWorkspace");
    outWS = execSpecifyGroupsManually(inputWS);
  } else {
    const double alpha = static_cast<double>(getProperty("Alpha"));
    MatrixWorkspace_sptr ws1 = getProperty("InputWorkspace1");
    MatrixWorkspace_sptr ws2 = getProperty("InputWorkspace2");
    // TODO : basic checks on workspaces.
    outWS = createPairWorkspaceFromGroupWorkspaces(ws1, ws2, alpha);
  }

  setPairAsymmetrySampleLogs(outWS);
  setProperty("OutputWorkspace", outWS);
}

MatrixWorkspace_sptr
MuonPairingAsymmetry::execSpecifyGroupsManually(WorkspaceGroup_sptr inputWS) {

  const double alpha = static_cast<double>(getProperty("Alpha"));
  std::vector<int> summedPeriods = getProperty("SummedPeriods");
  std::vector<int> subtractedPeriods = getProperty("SubtractedPeriods");

  auto summedWS = MuonAlgorithmHelper::sumPeriods(inputWS, summedPeriods);
  auto subtractedWS =
      MuonAlgorithmHelper::sumPeriods(inputWS, subtractedPeriods);

  MatrixWorkspace_sptr asymSummedPeriods = asymmetryCalc(summedWS, alpha);

  if (subtractedPeriods.empty()) {
    return asymSummedPeriods;
  }

  MatrixWorkspace_sptr asymSubtractedPeriods =
      asymmetryCalc(subtractedWS, alpha);

  return MuonAlgorithmHelper::subtractWorkspaces(asymSummedPeriods,
                                                 asymSubtractedPeriods);
}

/**
 * calculate asymmetry for a pair of workspaces of grouped detectors, using
 * parameter alpha, returning the resulting workspace.
 */
MatrixWorkspace_sptr
MuonPairingAsymmetry::createPairWorkspaceFromGroupWorkspaces(
    MatrixWorkspace_sptr inputWS1, MatrixWorkspace_sptr inputWS2,
    const double &alpha) {

  IAlgorithm_sptr alg = this->createChildAlgorithm("AppendSpectra");
  alg->setProperty("InputWorkspace1", inputWS1);
  alg->setProperty("InputWorkspace2", inputWS2);
  alg->setProperty("ValidateInputs", true);
  alg->execute();
  MatrixWorkspace_sptr ws = alg->getProperty("OutputWorkspace");

  ws = asymmetryCalc(ws, alpha);

  return ws;
}

/**
 * Performs asymmetry calculation on the given workspace using indices 0,1.
 * @param inputWS :: [input] Workspace to calculate asymmetry from (will use
 * workspace indices 0,1).
 * @returns MatrixWorkspace containing result of the calculation.
 */
MatrixWorkspace_sptr
MuonPairingAsymmetry::asymmetryCalc(MatrixWorkspace_sptr inputWS,
                                    const double &alpha) {
  MatrixWorkspace_sptr outWS;

  // Ensure our specified spectra definitely point to the data
  inputWS->getSpectrum(0).setSpectrumNo(0);
  inputWS->getSpectrum(1).setSpectrumNo(1);
  std::vector<int> fwdSpectra = {0};
  std::vector<int> bwdSpectra = {1};

  IAlgorithm_sptr alg = this->createChildAlgorithm("AsymmetryCalc");
  alg->setChild(true);
  alg->setProperty("InputWorkspace", inputWS);
  alg->setProperty("ForwardSpectra", fwdSpectra);
  alg->setProperty("BackwardSpectra", bwdSpectra);
  alg->setProperty("Alpha", alpha);
  alg->setProperty("OutputWorkspace", "__NotUsed__");
  alg->execute();
  outWS = alg->getProperty("OutputWorkspace");

  return outWS;
}

void MuonPairingAsymmetry::setPairAsymmetrySampleLogs(
    MatrixWorkspace_sptr workspace) {
  MuonAlgorithmHelper::addSampleLog(workspace, "analysis_pairName",
                                    getProperty("PairName"));
  MuonAlgorithmHelper::addSampleLog(workspace, "analysis_alpha",
                                    getProperty("Alpha"));
  MuonAlgorithmHelper::addSampleLog(workspace, "analysis_group1",
                                    getProperty("Group1"));
  MuonAlgorithmHelper::addSampleLog(workspace, "analysis_group2",
                                    getProperty("Group2"));
  MuonAlgorithmHelper::addSampleLog(workspace, "analysis_periods_summed",
                                    getPropertyValue("SummedPeriods"));
  MuonAlgorithmHelper::addSampleLog(workspace, "analysis_periods_subtracted",
                                    getPropertyValue("SubtractedPeriods"));
}

} // namespace Muon
} // namespace Mantid
