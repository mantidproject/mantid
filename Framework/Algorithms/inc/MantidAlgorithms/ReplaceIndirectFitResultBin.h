// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_REPLACEINDIRECTFITRESULTBIN_H_
#define MANTID_ALGORITHMS_REPLACEINDIRECTFITRESULTBIN_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/**
  During a sequential fit in Indirect Data Analysis, the parameters fitted for a
  spectrum become the start parameters for the next spectrum. This can be a
  problem if the next spectrum is not similar to the previous spectrum and will
  lead to a bad fit for that spectrum.

  ReplaceIndirectFitResultBin : This algorithm takes a results workspace of a
  sequential fit for multiple spectra (1), and a results workspace for a singly
  fit spectrum (2), and it will replace the corresponding bad bin value in
  workspace (1) with the bin found in workspace (2).
 */
class DLLExport ReplaceIndirectFitResultBin : public API::Algorithm {
public:
  std::string const name() const override;
  int version() const override;
  std::string const category() const override;
  std::string const summary() const override;

private:
  void init() override;
  std::map<std::string, std::string> validateInputs() override;
  void exec() override;

  Mantid::API::MatrixWorkspace_sptr
  cloneWorkspace(std::string const &inputName);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_REPLACEINDIRECTFITRESULTBIN_H_ */
