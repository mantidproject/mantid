// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
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

/** ReflectometryReductionOneAuto2 : Algorithm to run ReflectometryReductionOne,
attempting to pick instrument parameters for missing properties. Version 2.
*/
class MANTID_REFLECTOMETRY_DLL ReflectometryReductionOneAuto2 : public ReflectometryWorkflowBase2 {
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
  };

  class RebinParams {
  public:
    RebinParams(const double qMin, const bool qMinIsDefault, const double qMax, const bool qMaxIsDefault,
                const std::optional<double> &qStep)
        : m_qMin(qMin), m_qMinIsDefault(qMinIsDefault), m_qMax(qMax), m_qMaxIsDefault(qMaxIsDefault), m_qStep(qStep) {};

    double qMin() const { return m_qMin; };
    bool qMinIsDefault() const { return m_qMinIsDefault; }
    double qMax() const { return m_qMax; };
    bool qMaxIsDefault() const { return m_qMaxIsDefault; }
    double qStep() const { return *m_qStep; };
    bool hasQStep() const { return m_qStep.has_value(); }
    std::vector<double> asVector() const { return std::vector<double>{qMin(), qStep(), qMax()}; };

  private:
    double m_qMin;
    bool m_qMinIsDefault;
    double m_qMax;
    bool m_qMaxIsDefault;
    std::optional<double> m_qStep;
  };

  void init() override;
  void exec() override;
  std::string getRunNumberForWorkspaceGroup(const WorkspaceGroup_const_sptr &workspace);
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
  /// Rebin and scale a workspace in Q
  Mantid::API::MatrixWorkspace_sptr rebinAndScale(const Mantid::API::MatrixWorkspace_sptr &inputWS,
                                                  RebinParams const &params);
  /// Crop a workspace in Q
  MatrixWorkspace_sptr cropQ(const MatrixWorkspace_sptr &inputWS, RebinParams const &params);
  /// Populate algorithmic correction properties
  void populateAlgorithmicCorrectionProperties(const Mantid::API::IAlgorithm_sptr &alg,
                                               const Mantid::Geometry::Instrument_const_sptr &instrument);
  /// Get a polarization efficiencies workspace.
  std::tuple<API::MatrixWorkspace_sptr, std::string, std::string> getPolarizationEfficiencies();
  void applyPolarizationCorrection(std::string const &outputIvsLam);
  API::MatrixWorkspace_sptr getFloodWorkspace();
  void applyFloodCorrection(API::MatrixWorkspace_sptr const &flood, const std::string &propertyName);
  void applyFloodCorrections();
  double getPropertyOrDefault(const std::string &propertyName, const double defaultValue, bool &isDefault);
  void setOutputWorkspaces(WorkspaceNames const &outputGroupNames, std::vector<std::string> &IvsLamGroup,
                           std::vector<std::string> &IvsQBinnedGroup, std::vector<std::string> &IvsQGroup);
  WorkspaceNames getOutputNamesForGroups(const std::string &inputName, const std::string &runNumber,
                                         const size_t wsGroupNumber);
};

} // namespace Reflectometry
} // namespace Mantid
