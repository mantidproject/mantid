// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_LORENTZCORRECTION_H_
#define MANTID_ALGORITHMS_LORENTZCORRECTION_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** LorentzCorrection : Algorithm Performs a lorentz correction
  (sin(theta)^2)/(wavelength^4) on a MatrixWorkspace in units of wavelength
*/
class DLLExport LorentzCorrection : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"AnvredCorrection"};
  }
  const std::string category() const override;
  const std::string summary() const override;
  std::map<std::string, std::string> validateInputs() override;

private:
  void init() override;
  void exec() override;
  void processTOF_SCD(Mantid::API::MatrixWorkspace_sptr &wksp,
                      Mantid::API::Progress &prog);
  void processTOF_PD(Mantid::API::MatrixWorkspace_sptr &wksp,
                     Mantid::API::Progress &prog);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_LORENTZCORRECTION_H_ */
