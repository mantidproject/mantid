// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_GETEIMONDET3_H_
#define MANTID_ALGORITHMS_GETEIMONDET3_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

namespace Mantid {
namespace Indexing {
class SpectrumIndexSet;
}
namespace Algorithms {

/** Estimates the incident neutron energy from the time of flight
    between a monitor and a set of detectors.
*/
class DLLExport GetEiMonDet3 final : public API::Algorithm {
public:
  const std::string name() const override;
  const std::string summary() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override;
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
  double computeTOF(const API::MatrixWorkspace &detectorWs,
                    const double detectorEPP, const double monitorEPP,
                    const double minTOF);
  API::MatrixWorkspace_sptr groupSpectra(API::MatrixWorkspace_sptr &ws,
                                         const std::vector<size_t> &wsIndices);
  double minimumTOF(const API::MatrixWorkspace &ws,
                    const double sampleToDetectorDistance);
  double monitorPeakPosition(const size_t monitorIndex);
  double peakPosition(API::MatrixWorkspace_sptr &ws);
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_GETEIMONDET3_H_*/
