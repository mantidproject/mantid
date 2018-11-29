// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONEAUTO2_H_
#define MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONEAUTO2_H_

#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "ReflectometryWorkflowBase2.h"

namespace Mantid {
namespace Algorithms {

/** ReflectometryReductionOneAuto2 : Algorithm to run ReflectometryReductionOne,
attempting to pick instrument parameters for missing properties. Version 2.
*/
class DLLExport ReflectometryReductionOneAuto2
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
  void init() override;
  void exec() override;
  // Set default names for output workspaces
  void setDefaultOutputWorkspaceNames();
  /// Get the name of the detectors of interest based on processing instructions
  std::vector<std::string>
  getDetectorNames(Mantid::API::MatrixWorkspace_sptr inputWS);
  /// Correct detector positions vertically
  Mantid::API::MatrixWorkspace_sptr
  correctDetectorPositions(Mantid::API::MatrixWorkspace_sptr inputWS,
                           const double twoTheta);
  /// Calculate theta
  double calculateTheta(Mantid::API::MatrixWorkspace_sptr inputWS);
  /// Rebin and scale a workspace in Q
  Mantid::API::MatrixWorkspace_sptr
  rebinAndScale(Mantid::API::MatrixWorkspace_sptr inputWS, double theta,
                std::vector<double> &params);
  /// Populate algorithmic correction properties
  void populateAlgorithmicCorrectionProperties(
      Mantid::API::IAlgorithm_sptr alg,
      Mantid::Geometry::Instrument_const_sptr instrument);
  /// Get a polarization efficiencies workspace.
  std::tuple<API::MatrixWorkspace_sptr, std::string, std::string>
  getPolarizationEfficiencies();
  void applyPolarizationCorrection(std::string const &outputIvsLam);
  API::MatrixWorkspace_sptr getFloodWorkspace();
  void applyFloodCorrection(API::MatrixWorkspace_sptr const &flood,
                            const std::string &propertyName);
  void applyFloodCorrections();
  double getPropertyOrDefault(const std::string &propertyName,
                              const double defaultValue);
  void setOutputWorkspaces(std::vector<std::string> &IvsLamGroup,
                           std::string const &outputIvsLam,
                           std::vector<std::string> &IvsQGroup,
                           std::string const &outputIvsQBinned,
                           std::vector<std::string> &IvsQUnbinnedGroup,
                           std::string const &outputIvsQ);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONEAUTO2_H_ */
