// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef QUERYALLREMOTEJOBS2_H_
#define QUERYALLREMOTEJOBS2_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace RemoteAlgorithms {

/**
Query status information about all jobs running on a (remote) compute
resource.
*/
class DLLExport QueryAllRemoteJobs2 : public Mantid::API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "QueryAllRemoteJobs"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Query a remote compute resource for all jobs the user has "
           "submitted.";
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

#endif /*QUERYALLREMOTEJOBS2_H_*/
