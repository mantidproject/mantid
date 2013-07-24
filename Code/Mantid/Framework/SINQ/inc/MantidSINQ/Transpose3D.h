/**
 * This algorithm takes a 3D MD workspace and performs certain axis transposings on it.
 * Essentially this fixes some mess which developed at SINQ when being to hasty taking the
 * EMBL detectors into operation.
 *
 * I am afraid that this code has grown to do something else: I suspect that Mantids MDHistoWorkspace
 * is acting in F77 storage order. This, then, is also fixed here.
 *
 * Copyright: do not hold me or PSI responsible for anything or plug in
 * the Mantid copyright.
 *
 * Mark Koennecke, November 2012
 */
#ifndef TRANSPOSE3D_H_
#define TRANSPOSE3D_H_

#include "MantidAPI/Algorithm.h"
#include "MantidMDEvents/MDHistoWorkspace.h"

class Transpose3D : public Mantid::API::Algorithm
{
public:
  /// (Empty) Constructor
  Transpose3D() : Mantid::API::Algorithm() {}
  /// Virtual destructor
  virtual ~Transpose3D() {}
  /// Algorithm's name
  virtual const std::string name() const { return "Transpose3D"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "MDAlgorithms"; }

private:
  /// Initialisation code
  void init();
  ///Execution code
  void exec();

  void doYXZ( Mantid::API::IMDHistoWorkspace_sptr inws);
  void doXZY( Mantid::API::IMDHistoWorkspace_sptr inws);
  void doTRICS( Mantid::API::IMDHistoWorkspace_sptr inws);
  void doAMOR( Mantid::API::IMDHistoWorkspace_sptr inws);

  void copyMetaData( Mantid::API::IMDHistoWorkspace_sptr inws,  Mantid::API::IMDHistoWorkspace_sptr outws);
};

#endif /*TRANSPOSE3D_H_*/
