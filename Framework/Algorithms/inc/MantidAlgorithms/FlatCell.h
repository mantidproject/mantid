// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {

namespace Algorithms {
/** Generates the FlatCell workspace for ISIS SANS reduction of LOQ */
class MANTID_ALGORITHMS_DLL FlatCell : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "FlatCell"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Generates the FlatCell workspace for ISIS SANS reduction of LOQ.";
  }

  /// Algorithm's version
  int version() const override { return (1); }

  /// Algorithm's category for identification
  const std::string category() const override { return "Place\\Holder"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  void execEvent();
};

} // namespace Algorithms
} // namespace Mantid
