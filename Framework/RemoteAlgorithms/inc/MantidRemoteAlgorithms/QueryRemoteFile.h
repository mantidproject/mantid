#ifndef QUERYREMOTEFILE_H_
#define QUERYREMOTEFILE_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace RemoteAlgorithms {

class DLLExport QueryRemoteFile : public Mantid::API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "QueryRemoteFile"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Retrieve a list of the files from a remote compute resource.";
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
#endif /*QUERYREMOTEFILE_H_*/
