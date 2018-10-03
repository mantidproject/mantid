// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef ABORTREMOTEJOB2_H_
#define ABORTREMOTEJOB2_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace RemoteAlgorithms {

/**
Abort/cancel/kill a job running on a remote compute resource.
*/
class DLLExport AbortRemoteJob2 : public Mantid::API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "AbortRemoteJob"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Abort a previously submitted job.";
  }

  /// Algorithm's version
  int version() const override { return (2); }
  /// Algorithm's category for identification
  const std::string category() const override { return "Remote"; }

private:
  void init() override;
  /// Execution code
  void exec() override;
};

} // end namespace RemoteAlgorithms
} // end namespace Mantid

#endif /*ABORTREMOTEJOB2_H_*/
