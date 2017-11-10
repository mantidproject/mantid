#ifndef QUERYALLREMOTEJOBS_H_
#define QUERYALLREMOTEJOBS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"

namespace Mantid {
namespace RemoteAlgorithms {

class DLLExport QueryAllRemoteJobs : public Mantid::API::Algorithm,
                                     public API::DeprecatedAlgorithm {
public:
  /// Constructor
  QueryAllRemoteJobs() { this->useAlgorithm("QueryAllRemoteJobs", 2); }

  /// Algorithm's name
  const std::string name() const override { return "QueryAllRemoteJobs"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Query a remote compute resource for all jobs the user has "
           "submitted.";
  }

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
#endif /*QUERYALLREMOTEJOBS_H_*/
