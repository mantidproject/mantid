// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** Stitches overlapping spectra from multiple 2D workspaces.
 */
class MANTID_ALGORITHMS_DLL Stitch : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  std::map<std::string, std::string> validateInputs() override;

private:
  void init() override;
  void exec() override;
  void scale(API::MatrixWorkspace_sptr wsToMatch, API::MatrixWorkspace_sptr wsToScale,
             API::MatrixWorkspace_sptr scaleFactorsWorkspace, const std::vector<std::string> &inputs);
  API::MatrixWorkspace_sptr merge(const std::vector<std::string> &workspaces);
  void scaleManual(const std::vector<std::string> &inputs, const std::vector<double> &scaleFactors,
                   API::MatrixWorkspace_sptr scaleFactorsWorkspace);
  void scaleWithMedianRatios(const std::vector<std::string> &clones, const std::string &referenceName,
                             API::MatrixWorkspace_sptr &scaleFactorsWorkspace);
  void recordScaleFactor(API::MatrixWorkspace_sptr scaleFactorWorkspace, API::MatrixWorkspace_sptr medianWorkspace,
                         API::MatrixWorkspace_sptr scaledWorkspace, const std::vector<std::string> &inputs);
  void cloneWorkspaces(const std::vector<std::string> &inputs);
  size_t getReferenceIndex(const std::vector<API::MatrixWorkspace_sptr> &workspaces, const std::string &referenceName);
};

} // namespace Algorithms
} // namespace Mantid
