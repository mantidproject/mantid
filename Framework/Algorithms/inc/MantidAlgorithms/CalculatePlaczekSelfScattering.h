// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_CALCULATEPLACZEKSELFSCATTERING_H_
#define MANTID_ALGORITHMS_CALCULATEPLACZEKSELFSCATTERING_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** CalculatePlaczekSelfScattering : This algorithm calculates a correction for
  an incident spectrum defracted by a sample.
*/
class MANTID_ALGORITHMS_DLL CalculatePlaczekSelfScattering
    : public API::Algorithm {
public:
  CalculatePlaczekSelfScattering() : API::Algorithm() {}
  virtual ~CalculatePlaczekSelfScattering() {}
  virtual const std::string name() const override {
    return "CalculatePlaczekSelfScattering";
  }
  virtual int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"FitIncidentSpectrum"};
  }
  const std::string category() const override { return "CorrectionFunctions"; };
  const std::string summary() const override {
    return "Calculates the Placzek self scattering correction of an incident "
           "spectrum";
  };
  std::map<std::string, std::string> validateInputs() override;

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CALCULATEPLACZEKSELFSCATTERING_H_ */