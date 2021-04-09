// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace WorkflowAlgorithms {

/** DgsConvertToEnergyTransfer : This is the algorithm responsible for the
 * conversion from TOF to energy transfer.

@date 2012-07-11
 */
class DLLExport DgsConvertToEnergyTransfer : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Algorithm to convert from TOF to energy transfer."; }
  int version() const override;
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace WorkflowAlgorithms
} // namespace Mantid
