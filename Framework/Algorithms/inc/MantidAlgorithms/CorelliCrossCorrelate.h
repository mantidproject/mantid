// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid {
namespace Algorithms {

/** CorelliCrossCorrelate : TODO: DESCRIPTION
 */
class MANTID_ALGORITHMS_DLL CorelliCrossCorrelate : public API::Algorithm {
public:
  const std::string name() const override { return "CorelliCrossCorrelate"; };
  int version() const override { return 1; };
  const std::string category() const override { return "Diffraction\\Calibration;Events"; };
  const std::string summary() const override {
    return "Cross-correlation calculation for the elastic signal from Corelli.";
  };

private:
  std::map<std::string, std::string> validateInputs() override;
  void init() override;
  void exec() override;

  /// Input workspace
  DataObjects::EventWorkspace_const_sptr inputWS;
  DataObjects::EventWorkspace_sptr outputWS;
};

} // namespace Algorithms
} // namespace Mantid
