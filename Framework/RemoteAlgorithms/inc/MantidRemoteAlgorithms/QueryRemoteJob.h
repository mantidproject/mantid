#ifndef QUERYREMOTEJOB_H_
#define QUERYREMOTEJOB_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"

namespace Mantid {
namespace RemoteAlgorithms {

class DLLExport QueryRemoteJob : public Mantid::API::Algorithm,
                                 public API::DeprecatedAlgorithm {
public:
  /// Constructor
  QueryRemoteJob() { this->useAlgorithm("QueryRemoteJob", 2); }

  /// Algorithm's name
  const std::string name() const override { return "QueryRemoteJob"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Query a remote compute resource for a specific job";
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
#endif /*QUERYREMOTEJOB_H_*/
