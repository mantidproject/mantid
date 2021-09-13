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

class MANTID_ALGORITHMS_DLL MyNameCpp : public API::Algorithm {
public:
  const std::string name() const override { return "MyNameCpp"; }
  const std::string summary() const override { return "Testing alias deprecation"; }
  int version() const override { return 1; }
  const std::string category() const override { return "Testing"; }
  const std::string alias() const override { return "MyAliasCpp"; }
  const std::string aliasExpiration() const override { return "2020-02-01"; }

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
