// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_REMOVEMASKEDSPECTRA_H_
#define MANTID_ALGORITHMS_REMOVEMASKEDSPECTRA_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** RemoveMaskedSpectra removes all masked spectra.
 */
class DLLExport RemoveMaskedSpectra : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"ExtractUnmaskedSpectra"};
  }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
  void makeIndexList(std::vector<size_t> &indices,
                     const API::MatrixWorkspace *maskedWorkspace);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_REMOVEMASKEDSPECTRA_H_ */