// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef SUBMITREMOTEJOB2_H_
#define SUBMITREMOTEJOB2_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace RemoteAlgorithms {

/**
 Submit a job to be executed on a (remote) compute resource
*/
class DLLExport SubmitRemoteJob2 : public Mantid::API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "SubmitRemoteJob"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Submit a job to be executed on the specified remote compute "
           "resource.";
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

#endif /*SUBMITREMOTEJOB2_H_*/
