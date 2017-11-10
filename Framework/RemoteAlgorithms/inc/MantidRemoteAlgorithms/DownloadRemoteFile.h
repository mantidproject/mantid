#ifndef DOWNLOADREMOTEFILE_H_
#define DOWNLOADREMOTEFILE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"

namespace Mantid {
namespace RemoteAlgorithms {

class DLLExport DownloadRemoteFile : public Mantid::API::Algorithm,
                                     public API::DeprecatedAlgorithm {
public:
  /// constructor
  DownloadRemoteFile();

  /// Algorithm's name
  const std::string name() const override { return "DownloadRemoteFile"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Download a file from a remote compute resource.";
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
#endif /*DOWNLOADREMOTEFILE_H_*/
