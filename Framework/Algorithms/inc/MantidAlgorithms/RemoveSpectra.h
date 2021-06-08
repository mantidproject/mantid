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
#include "MantidIndexing/IndexInfo.h"

using namespace Mantid::API;
using namespace Mantid::Indexing;

namespace Mantid {
namespace Algorithms {

class MANTID_ALGORITHMS_DLL RemoveSpectra : public API::Algorithm {
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "RemoveSpectra"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Remove spectra from a given workspace, from a list, if spectra is "
           "masked, or if spectra has no detector.";
  }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"ExtractSpectra"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Transforms\\Splitting"; }

private:
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;
  MatrixWorkspace_sptr copySpectraFromInputToOutput(MatrixWorkspace_sptr inputWS, const std::vector<size_t> &specList);
};
} // namespace Algorithms
} // namespace Mantid
