// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_COPYDATARANGE_H_
#define MANTID_ALGORITHMS_COPYDATARANGE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/**
  CopyDataRange : This algorithm takes a continuous block of data
  from an input workspace specified by spectra indices and x indices and
  replaces a block of data within a destination workspace. Where this block of
  data is inserted is decided by an InsertionYIndex and an InsertionXIndex.
 */
class DLLExport CopyDataRange : public API::Algorithm {
public:
  std::string const name() const override;
  int version() const override;
  std::string const category() const override;
  std::string const summary() const override;

private:
  void init() override;
  std::map<std::string, std::string> validateInputs() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_COPYDATARANGE_H_ */
