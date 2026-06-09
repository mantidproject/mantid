// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidReflectometry/DllConfig.h"
#include "ReflectometryWorkflowBase2.h"

#include <optional>

namespace Mantid {
namespace Reflectometry {

/** ReflectometryReductionOneAuto3 : Algorithm to run ReflectometryReductionOne,
attempting to pick instrument parameters for missing properties. Version 3.
*/
class MANTID_REFLECTOMETRY_DLL ReflectometryReductionOneAuto3 : public ReflectometryWorkflowBase2 {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"ReflectometryReductionOne"}; }
  const std::string category() const override;
  const std::string summary() const override;

  /// Validate inputs
  std::map<std::string, std::string> validateInputs() override;

  /// For (multiperiod) workspace groups
  bool checkGroups() override;
  bool processGroups() override;

private:
  // Utility class to store output workspace names
  struct WorkspaceNames {
    std::string iVsQ;
    std::string iVsQBinned;
    std::string iVsLam;
    std::string trans;
    std::string trans1;
    std::string trans2;
  };

  // Utility struct to collate output properties of child algorithm when
  // processing groups
  struct OutputProperties {
    double thetaIn;
    double qMin;
    double qMax;
    double qStep;
    double scale;
  };

  struct RebinParams {
    double qMin;
    bool qMinIsDefault;
    double qMax;
    bool qMaxIsDefault;
    std::optional<double> qStep;

    bool hasQStep() const { return qStep.has_value(); }
    std::vector<double> asVector() const { return {qMin, *qStep, qMax}; }
  };

  struct RROOutputs {
    MatrixWorkspace_sptr IvsQ;
    MatrixWorkspace_sptr IvsLam;
    MatrixWorkspace_sptr trans;
    MatrixWorkspace_sptr trans1;
    MatrixWorkspace_sptr trans2;
    MatrixWorkspace_sptr out;
    double theta;
  };

  struct processGroupMembersOutput {
    std::vector<RROOutputs> rroOutputs;
    std::vector<WorkspaceNames> outputNames;
  };

  struct CorrectionProperties {
    std::string type;
    std::string c0;
    std::string c1;
    std::string polynomial;
  };

  // A class member to cache properties associated with the algorithmic correction to be applied
  CorrectionProperties m_correctionProperties;

  void init() override;
  void exec() override;
  std::string getRunNumberForWorkspaceGroup(std::string const &wsName);
  WorkspaceNames getOutputWorkspaceNames();
  void setDefaultOutputWorkspaceNames();
  /// Get the name of the detectors of interest based on processing instructions
  std::vector<std::string> getDetectorNames(const Mantid::API::MatrixWorkspace_sptr &inputWS);
  /// Correct detector positions vertically
  Mantid::API::MatrixWorkspace_sptr correctDetectorPositions(Mantid::API::MatrixWorkspace_sptr inputWS,
                                                             const double twoTheta);
  /// Calculate theta
  double calculateTheta(const Mantid::API::MatrixWorkspace_sptr &inputWS);
  /// Find cropping and binning parameters
  RebinParams getRebinParams(const MatrixWorkspace_sptr &inputWS, const double theta);
  std::optional<double> getQStep(const MatrixWorkspace_sptr &inputWS, const double theta);
  // Optionally scale a workspace
  Mantid::API::MatrixWorkspace_sptr scale(Mantid::API::MatrixWorkspace_sptr inputWS);
  /// Rebin a workspace in Q
  Mantid::API::MatrixWorkspace_sptr rebin(const Mantid::API::MatrixWorkspace_sptr &inputWS, const RebinParams &params);
  /// Optionally crop a workspace in Q
  MatrixWorkspace_sptr cropQ(MatrixWorkspace_sptr inputWS, const RebinParams &params);
  /// Populate algorithmic correction properties
  void determineCorrectionAlgorithm(const Instrument_const_sptr &instrument);
  void populateAlgorithmicCorrectionProperties(const Mantid::API::IAlgorithm_sptr &alg);
  std::tuple<API::MatrixWorkspace_sptr, std::string, std::string, std::string>
  getPolarizationEfficiencies(const WorkspaceGroup_sptr &groupIvsLam);
  WorkspaceGroup_sptr applyPolarizationCorrection(const WorkspaceGroup_sptr &outputIvsLam,
                                                  const std::string &outputGroupName);
  API::MatrixWorkspace_sptr getFloodWorkspace(const Instrument_const_sptr &instrument);
  std::string getSummedWorkspaceName(const std::string &wsPropertyName, const bool isTransWs = false);
  void sumBanksForWorkspace(const std::string &roiDetectorIDs, const std::string &wsPropertyName,
                            const bool isTransWs = false);
  void sumBanks();
  double getPropertyOrDefault(const std::string &propertyName, const double defaultValue, bool &isDefault);
  WorkspaceNames getOutputNamesForGroupMember(const std::string &inputNames, const std::string &runNumber,
                                              const size_t wsGroupNumber);
  void validateTransmissionRun(std::map<std::string, std::string> &results, const std::string &transmissionRun);
  void setOutputGroupedWorkspaces(std::vector<WorkspaceNames> const &outputNames,
                                  WorkspaceNames const &outputGroupNames, const bool outputIvsLam, bool outputTrans,
                                  bool outputTrans1, bool outputTrans2);
  void setOutputPropertyFromChild(const Algorithm_sptr &alg, std::string const &name);
  void setOutputPropertiesFromChild(const Algorithm_sptr &alg);
  processGroupMembersOutput processGroupMembers(const Algorithm::WorkspaceVector &members, std::string const &runNumber,
                                                std::vector<std::string> const &taskOrder, const bool reduced = false);
  WorkspaceGroup_sptr groupWorkspaces(const std::vector<std::string> &workspaceNames,
                                      std::string const &outputName = "");
  RROOutputs performCoreReduction(MatrixWorkspace_sptr inputWS, const std::vector<std::string> &taskOrder = {},
                                  const bool applyFloodCorrections = true);
  MatrixWorkspace_sptr postReductionProcessing(const RROOutputs &out, const RebinParams &params);
  void postReductionProcessingGroups(std::vector<RROOutputs> &outputs, std::vector<WorkspaceNames> const &outputNames,
                                     const WorkspaceNames &groupedOutputNames, const bool outputIvsLam);
  void setOutputWorkspaces(const RROOutputs &out, const MatrixWorkspace_sptr &binnedWS);
  void updatePropertiesAfterReduction(RROOutputs &out, const RebinParams &params);
  std::vector<std::string> getTaskExecutionOrder(const bool reduced, const bool summingInQ) const;
};
} // namespace Reflectometry
} // namespace Mantid
