// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_MAYERSSAMPLECORRECTION_H_
#define MANTID_ALGORITHMS_MAYERSSAMPLECORRECTION_H_
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {

/**
  Corrects for the effects of absorption and multiple scattering using the
  algorithm of Jerry Mayers.
  See https://inis.iaea.org/search/search.aspx?orig_q=RN:20000574
*/
class DLLExport MayersSampleCorrection : public API::Algorithm {
public:
  MayersSampleCorrection();

  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"MonteCarloAbsorption", "CarpenterSampleCorrection"};
  }

private:
  void init() override;
  void exec() override;

  Kernel::IValidator_sptr createInputWSValidator() const;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_MAYERSSAMPLECORRECTION_H_ */
