#ifndef SUBMITREMOTEJOB_H_
#define SUBMITREMOTEJOB_H_

#include "MantidAPI/Algorithm.h"

class SubmitRemoteJob : public Mantid::API::Algorithm
{
public:
  /// (Empty) Constructor
  SubmitRemoteJob() : Mantid::API::Algorithm() {}
  /// Virtual destructor
  virtual ~SubmitRemoteJob() {}
  /// Algorithm's name
  virtual const std::string name() const { return "SubmitRemoteJob"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "RemoteAlgorithms"; }

private:
  /// Initialisation code
  void init();
  ///Execution code
  void exec();

};

#endif /*SUBMITREMOTEJOB_H_*/
