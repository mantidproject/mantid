// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTID_ALGORITHMS_REMOVESPECTRA_H_
#define MANTID_ALGORITHMS_REMOVESPECTRA_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {

class DLLExport RemoveSpectra : public API::Algorithm {
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "RemoveSpectra"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Remove spectra from a given workspace, from a list, if spectra is "
           "masked, or if spectra has no detector.";
  }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"RemoveSpectra"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Utility\\Workspaces"; }

private:
  void init() override;
  void exec() override;
};
} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_REMOVESPECTRA_H_ */