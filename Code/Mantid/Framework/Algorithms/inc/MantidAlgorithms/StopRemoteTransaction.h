#ifndef STOPREMOTETRANSACTION_H_
#define STOPREMOTETRANSACTION_H_

#include "MantidAPI/Algorithm.h"

class StopRemoteTransaction : public Mantid::API::Algorithm
{
public:
  /// (Empty) Constructor
  StopRemoteTransaction() : Mantid::API::Algorithm() {}
  /// Virtual destructor
  virtual ~StopRemoteTransaction() {}
  /// Algorithm's name
  virtual const std::string name() const { return "StopRemoteTransaction"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "UserAlgorithms"; }

private:
  /// Initialisation code
  void init();
  ///Execution code
  void exec();

};

#endif /*STOPREMOTETRANSACTION_H_*/
