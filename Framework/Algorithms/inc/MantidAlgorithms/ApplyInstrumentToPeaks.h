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

/** ApplyInstrumentToPeaks : TODO: DESCRIPTION
 */
class MANTID_ALGORITHMS_DLL ApplyInstrumentToPeaks final : public API::Algorithm {
public:
  const std::string name() const override { return "ApplyInstrumentToPeaks"; }
  int version() const override { return 1; }
  const std::string category() const override { return "Crystal\\Peaks"; }
  const std::string summary() const override {
    return "Update the instrument attached to peaks within a PeaksWorkspace to match the one provided or the one "
           "attached to the input workspace while keeping detectorID and TOF unchanged";
  }

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
