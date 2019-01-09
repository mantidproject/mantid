// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
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
  /// Returns algorithm's name for identification

  const std::string name() const override;

  /// Returns a summary of algorithm's purpose
  const std::string summary() const override;
  /// Returns algorithm's version for identification
  int version() const override;

  const std::vector<std::string> seeAlso() const override;

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override;

private:
  /// Initializes the algorithm
  void init() override;

  /// Executes the algorithm
  void exec() override;
  /// Calculates the average sample-to-detector distance and TOF

  /// Calculates the total TOF from monitor to detectors
  double computeTOF(const API::MatrixWorkspace &detectorWs, const double detectorEPP, const double monitorEPP);

  API::MatrixWorkspace_sptr groupSpectra(API::MatrixWorkspace_sptr &ws, const std::vector<int> &wsIndices);
  double peakPosition(API::MatrixWorkspace_sptr &ws);
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_GETEIMONDET3_H_*/
