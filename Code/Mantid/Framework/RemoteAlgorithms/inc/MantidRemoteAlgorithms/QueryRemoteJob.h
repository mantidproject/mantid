#ifndef QUERYREMOTEJOB_H_
#define QUERYREMOTEJOB_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace RemoteAlgorithms {

class QueryRemoteJob : public Mantid::API::Algorithm {
public:
  /// (Empty) Constructor
  QueryRemoteJob() : Mantid::API::Algorithm() {}
  /// Virtual destructor
  virtual ~QueryRemoteJob() {}
  /// Algorithm's name
  virtual const std::string name() const { return "QueryRemoteJob"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Query a remote compute resource for a specific job";
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
#endif /*QUERYREMOTEJOB_H_*/
