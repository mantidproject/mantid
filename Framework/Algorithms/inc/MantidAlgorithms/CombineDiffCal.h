// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidDataObjects/TableWorkspace.h"

namespace Mantid {
namespace Algorithms {

/** CombineDiffCal : Calibrate groups of pixels after cross correlation so that diffraction peaks can be adjusted to the
 * correct positions
 */
class MANTID_ALGORITHMS_DLL CombineDiffCal final : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  std::map<std::string, std::string> validateInputs() override;

private:
  DataObjects::TableWorkspace_sptr sortTableWorkspace(DataObjects::TableWorkspace_sptr const &table);
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
