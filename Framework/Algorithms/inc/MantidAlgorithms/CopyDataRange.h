// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/**
  CopyDataRange : This algorithm takes a continuous block of data
  from an input workspace specified by spectra indices and x indices and
  replaces a block of data within a destination workspace. Where this block of
  data is inserted is decided by an InsertionYIndex and an InsertionXIndex.
 */
class MANTID_ALGORITHMS_DLL CopyDataRange : public API::Algorithm {
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
