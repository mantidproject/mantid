// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidCrystal/DllConfig.h"
#include "MantidDataObjects/PeaksWorkspace.h"

namespace Mantid {
namespace Crystal {

/** ConvertPeaksWorkspace :
 *  Bi-directional conversion between a regular PeaksWorkspace (with instrument) and a LeanElasticPeaksWorkspace (no
 * instrument)
 *
 *  Details:
 *  - PeaksWorkspace -> LeanElasticPeaksWorkspace
 *    - all Peaks are casted to LeanElasticPeaks
 *    - all instrument related information is lost
 *  - LeanElasticPeaksWorkspace -> PeaksWorkspace
 *    - requires a donor workspace to provide insturment information
 *    - all LeanElasticPeaks are casted to Peaks
 *       - peaks with negative wavelength is skipped (aka losing some peaks)
 *       - the instrument from the donor workspace is added to each individual Peak
 */
class MANTID_CRYSTAL_DLL ConvertPeaksWorkspace : public Mantid::API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "ConvertPeaksWorkspace"; }

  /// Summary of algorithm's purpose
  const std::string summary() const override {
    return "Bi-directional conversion between a regular PeaksWorkspace (with instrument) and a "
           "LeanElasticPeaksWorkspace (without instrument).";
  }

  /// Algorithm's version, overriding a virtual method
  int version() const override { return 1; }

  /// Algorithm's category, overriding a virtual method
  const std::string category() const override {
    return "Crystal\\Peaks;Utility\\Workspaces";
    ;
  }

  /// Extra help info
  const std::vector<std::string> seeAlso() const override { return {"CreatePeaksWorkspace"}; }

private:
  /// Initialise the properties
  void init() override;

  /// Run the algorithm
  void exec() override;

  /// Private validator for inputs
  std::map<std::string, std::string> validateInputs() override;

  /// PeaksWorkspace -> LeanElasticPeaksWorkspace
  Mantid::API::IPeaksWorkspace_sptr makeLeanElasticPeaksWorkspace(Mantid::API::IPeaksWorkspace_sptr ipws);

  /// LeanElasticPeaksWorkspace -> PeaksWorkspace
  Mantid::API::IPeaksWorkspace_sptr makePeaksWorkspace(Mantid::API::IPeaksWorkspace_sptr ipws,
                                                       Mantid::API::Workspace_sptr ws);
};

} // namespace Crystal
} // namespace Mantid
