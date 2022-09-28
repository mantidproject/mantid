// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** CalculatePlaczekSelfScattering2 : TODO: DESCRIPTION
 */
class MANTID_ALGORITHMS_DLL CalculatePlaczekSelfScattering2 final : public API::Algorithm {
public:
  CalculatePlaczekSelfScattering2() : API::Algorithm() {}
  virtual const std::string name() const override { return "CalculatePlaczekSelfScattering"; }
  virtual int version() const override { return (2); }
  const std::vector<std::string> seeAlso() const override { return {"FitIncidentSpectrum", "CalculatePlaczek"}; }
  const std::string category() const override { return "CorrectionFunctions"; };
  const std::string summary() const override {
    return "Calculates the Placzek self scattering correction of an incident "
           "spectrum";
  };

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
