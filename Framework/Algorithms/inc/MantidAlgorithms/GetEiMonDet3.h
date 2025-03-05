// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Indexing {
class SpectrumIndexSet;
}
namespace Algorithms {

/** Estimates the incident neutron energy from the time of flight
    between a monitor and a set of detectors.
*/
class MANTID_ALGORITHMS_DLL GetEiMonDet3 final : public API::Algorithm {
public:
  const std::string name() const override;
  const std::string summary() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override;
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
  double computeTOF(const API::MatrixWorkspace &detectorWs, const double detectorEPP, const double monitorEPP,
                    const double minTOF);
  API::MatrixWorkspace_sptr groupSpectra(const API::MatrixWorkspace_sptr &ws, const std::vector<size_t> &wsIndices);
  double minimumTOF(const API::MatrixWorkspace &ws, const double sampleToDetectorDistance);
  double monitorPeakPosition(const size_t monitorIndex);
  double peakPosition(const API::MatrixWorkspace_sptr &ws);
};

} // namespace Algorithms
} // namespace Mantid
