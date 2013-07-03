#ifndef STARTREMOTETRANSACTION_H_
#define STARTREMOTETRANSACTION_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace RemoteAlgorithms {

class StartRemoteTransaction : public Mantid::API::Algorithm
{
public:
  /// (Empty) Constructor
  StartRemoteTransaction() : Mantid::API::Algorithm() {}
  /// Virtual destructor
  virtual ~StartRemoteTransaction() {}
  /// Algorithm's name
  virtual const std::string name() const { return "StartRemoteTransaction"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Remote"; }

private:
  /// Initialisation code
  void init();
  ///Execution code
  void exec();

};

} // end namespace RemoteAlgorithms
} // end namespace Mantid
#endif /*STARTREMOTETRANSACTION_H_*/
