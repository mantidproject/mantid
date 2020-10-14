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

#include <boost/optional.hpp>

namespace Mantid {
namespace Reflectometry {

/** ReflectometryReductionOneAuto3 : Algorithm to run ReflectometryReductionOne,
attempting to pick instrument parameters for missing properties. Version 3.
*/
class MANTID_REFLECTOMETRY_DLL ReflectometryReductionOneAuto3
    : public ReflectometryWorkflowBase2 {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"ReflectometryReductionOne"};
  }
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
  };

  struct RebinParams {
    double qMin;
    bool qMinIsDefault;
    double qMax;
    bool qMaxIsDefault;
    boost::optional<double> qStep;

    bool hasQStep() const { return qStep.is_initialized(); }
    std::vector<double> asVector() const { return {qMin, *qStep, qMax}; }
  };

  void init() override;
  void exec() override;
  std::string
  getRunNumberForWorkspaceGroup(const WorkspaceGroup_const_sptr &workspace);
  WorkspaceNames getOutputWorkspaceNames();
  void setDefaultOutputWorkspaceNames();
  /// Get the name of the detectors of interest based on processing instructions
  std::vector<std::string>
  getDetectorNames(const Mantid::API::MatrixWorkspace_sptr &inputWS);
  /// Correct detector positions vertically
  Mantid::API::MatrixWorkspace_sptr
  correctDetectorPositions(Mantid::API::MatrixWorkspace_sptr inputWS,
                           const double twoTheta);
  /// Calculate theta
  double calculateTheta(const Mantid::API::MatrixWorkspace_sptr &inputWS);
  /// Find cropping and binning parameters
  RebinParams getRebinParams(const MatrixWorkspace_sptr &inputWS,
                             const double theta);
  boost::optional<double> getQStep(const MatrixWorkspace_sptr &inputWS,
                                   const double theta);
  // Optionally scale a workspace
  Mantid::API::MatrixWorkspace_sptr
  scale(Mantid::API::MatrixWorkspace_sptr inputWS);
  /// Rebin a workspace in Q
  Mantid::API::MatrixWorkspace_sptr
  rebin(const Mantid::API::MatrixWorkspace_sptr &inputWS,
        const RebinParams &params);
  /// Optionally crop a workspace in Q
  MatrixWorkspace_sptr cropQ(MatrixWorkspace_sptr inputWS,
                             const RebinParams &params);
  /// Populate algorithmic correction properties
  void populateAlgorithmicCorrectionProperties(
      const Mantid::API::IAlgorithm_sptr &alg,
      const Mantid::Geometry::Instrument_const_sptr &instrument);
  /// Get a polarization efficiencies workspace.
  std::tuple<API::MatrixWorkspace_sptr, std::string, std::string>
  getPolarizationEfficiencies();
  void applyPolarizationCorrection(const std::string &outputIvsLam);
  API::MatrixWorkspace_sptr getFloodWorkspace();
  void applyFloodCorrection(const API::MatrixWorkspace_sptr &flood,
                            const std::string &propertyName);
  void applyFloodCorrections();
  double getPropertyOrDefault(const std::string &propertyName,
                              const double defaultValue, bool &isDefault);
  void setOutputWorkspaces(const WorkspaceNames &outputGroupNames,
                           std::vector<std::string> &IvsLamGroup,
                           std::vector<std::string> &IvsQBinnedGroup,
                           std::vector<std::string> &IvsQGroup);
  WorkspaceNames getOutputNamesForGroups(const std::string &inputName,
                                         const std::string &runNumber,
                                         const size_t wsGroupNumber);
  void getTransmissionRun(std::map<std::string, std::string> &results,
                          WorkspaceGroup_sptr &workspaceGroup,
                          const std::string &transmissionRun);
};

} // namespace Reflectometry
} // namespace Mantid
