#ifndef QUERYALLREMOTEJOBS_H_
#define QUERYALLREMOTEJOBS_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace RemoteAlgorithms {

class DLLExport QueryAllRemoteJobs : public Mantid::API::Algorithm {
public:
  /// (Empty) Constructor
  QueryAllRemoteJobs() : Mantid::API::Algorithm() {}
  /// Virtual destructor
  virtual ~QueryAllRemoteJobs() {}
  /// Algorithm's name
  virtual const std::string name() const { return "QueryAllRemoteJobs"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Query a remote compute resource for all jobs the user has "
           "submitted.";
  }

  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Remote"; }

private:
  void init();
  /// Execution code
  void exec();
};

} // end namespace RemoteAlgorithms
} // end namespace Mantid
#endif /*QUERYALLREMOTEJOBS_H_*/
