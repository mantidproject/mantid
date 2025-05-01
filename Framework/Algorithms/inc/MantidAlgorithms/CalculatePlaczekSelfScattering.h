// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** CalculatePlaczekSelfScattering : This algorithm calculates a correction for
  an incident spectrum defracted by a sample.
*/
class MANTID_ALGORITHMS_DLL CalculatePlaczekSelfScattering final : public API::Algorithm {
public:
  CalculatePlaczekSelfScattering() : API::Algorithm() {}
  virtual const std::string name() const override { return "CalculatePlaczekSelfScattering"; }
  virtual int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"FitIncidentSpectrum"}; }
  const std::string category() const override { return "CorrectionFunctions"; };
  const std::string summary() const override {
    return "Calculates the Placzek self scattering correction of an incident "
           "spectrum";
  };
  std::map<std::string, std::string> validateInputs() override;

private:
  void init() override;
  void exec() override;
  double getPackingFraction(const API::MatrixWorkspace_const_sptr &ws);
};

} // namespace Algorithms
} // namespace Mantid
