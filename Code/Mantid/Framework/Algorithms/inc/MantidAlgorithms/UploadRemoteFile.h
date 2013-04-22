#ifndef UPLOADREMOTEFILE_H_
#define UPLOADREMOTEFILE_H_

#include "MantidAPI/Algorithm.h"

class UploadRemoteFile : public Mantid::API::Algorithm
{
public:
  /// (Empty) Constructor
  UploadRemoteFile() : Mantid::API::Algorithm() {}
  /// Virtual destructor
  virtual ~UploadRemoteFile() {}
  /// Algorithm's name
  virtual const std::string name() const { return "UploadRemoteFile"; }
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

#endif /*UPLOADREMOTEFILE_H_*/
