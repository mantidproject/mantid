// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Geometry {
class Instrument;
}
namespace Algorithms {

/** DirectILLTubeBackground : Fits polynomial backgrounds over the pixels of
  position sensitive tubes.
*/
class MANTID_ALGORITHMS_DLL DirectILLTubeBackground final : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::vector<std::string> seeAlso() const override;
  const std::string summary() const override;

private:
  void init() override;
  std::map<std::string, std::string> validateInputs() override;
  void exec() override;
  API::MatrixWorkspace_sptr applyDiagnostics(API::MatrixWorkspace_sptr ws);
  std::vector<std::string> components(Geometry::Instrument const &instrument);
  API::MatrixWorkspace_sptr cropToComponent(API::MatrixWorkspace_sptr const &ws, std::string const &componentName);
  API::MatrixWorkspace_sptr fitComponentBackground(API::MatrixWorkspace_sptr const &ws,
                                                   std::vector<double> const &xRanges);
  API::MatrixWorkspace_sptr peakExcludingAverage(const API::MatrixWorkspace &ws, std::vector<double> const &peakStarts,
                                                 std::vector<double> const &peakEnds);
};

} // namespace Algorithms
} // namespace Mantid
