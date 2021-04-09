// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

class MANTID_ALGORITHMS_DLL DeleteLog : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Removes a named log from a run"; }

  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"AddSampleLog", "RenameLog"}; }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
