#ifndef LOADEVENTPRENEXUS_H_
#define LOADEVENTPRENEXUS_H_

#include "MantidAPI/Algorithm.h"

class LoadEventPreNeXus : public Mantid::API::Algorithm
{
public:
  /// (Empty) Constructor
  LoadEventPreNeXus() : Mantid::API::Algorithm() {}
  /// Virtual destructor
  virtual ~LoadEventPreNeXus() {}
  /// Algorithm's name
  virtual const std::string name() const { return "LoadEventPreNeXus"; }
  /// Algorithm's version
  virtual const int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "DataHandling"; }

private:
  /// Initialisation code
  void init();
  ///Execution code
  void exec();

};

#endif /*LOADEVENTPRENEXUS_H_*/
