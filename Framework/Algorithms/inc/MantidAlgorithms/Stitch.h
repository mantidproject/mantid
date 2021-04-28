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

/** Stitches overlapping spectra from multiple workspaces.
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
  void scale(Mantid::API::MatrixWorkspace_sptr wsToMatch, Mantid::API::MatrixWorkspace_sptr wsToScale);
  Mantid::API::MatrixWorkspace_sptr merge(const std::vector<std::string> &workspaces);
};

} // namespace Algorithms
} // namespace Mantid
