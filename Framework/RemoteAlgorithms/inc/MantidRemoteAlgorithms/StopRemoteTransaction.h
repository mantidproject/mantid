// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"

namespace Mantid {
namespace RemoteAlgorithms {

class DLLExport StopRemoteTransaction : public Mantid::API::Algorithm, public API::DeprecatedAlgorithm {
public:
  /// Constructor
  StopRemoteTransaction() { this->useAlgorithm("StopRemoteTransaction", 2); }

  /// Algorithm's name
  const std::string name() const override { return "StopRemoteTransaction"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Stop a job transaction on a remote compute resource."; }

  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override { return "Remote"; }

private:
  void init() override;
  /// Execution code
  void exec() override;
};

} // end namespace RemoteAlgorithms
} // end namespace Mantid