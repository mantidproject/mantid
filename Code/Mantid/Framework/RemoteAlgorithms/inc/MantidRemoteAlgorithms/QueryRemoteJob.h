#ifndef QUERYREMOTEJOB_H_
#define QUERYREMOTEJOB_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace RemoteAlgorithms {

class QueryRemoteJob : public Mantid::API::Algorithm
{
public:
  /// (Empty) Constructor
  QueryRemoteJob() : Mantid::API::Algorithm() {}
  /// Virtual destructor
  virtual ~QueryRemoteJob() {}
  /// Algorithm's name
  virtual const std::string name() const { return "QueryRemoteJob"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Remote"; }

private:
  /// Initialisation code
  void init();
  ///Execution code
  void exec();
  /// Helper func for converting RemoteJob::JobStatus enums into strings
  std::string mapStatusToString( unsigned status);

};

} // end namespace RemoteAlgorithms
} // end namespace Mantid
#endif /*QUERYREMOTEJOB_H_*/
