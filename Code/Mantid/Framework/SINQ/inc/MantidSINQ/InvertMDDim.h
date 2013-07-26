/**
 * This Algorithms inverts the dimensions of a MD data set. The
 * application area is when fixing up MD workspaces which had to have the
 * dimensions inverted because they were delivered in C storage order.
 *
 * copyright: leave me alone or mantid Copyright
 *
 * Mark Koennecke, Dezember 2012
 */
#ifndef INVERTMDDIM_H_
#define INVERTMDDIM_H_

#include "MantidAPI/Algorithm.h"
#include "MantidMDEvents/MDHistoWorkspace.h"

class InvertMDDim : public Mantid::API::Algorithm
{
public:
  /// (Empty) Constructor
  InvertMDDim() : Mantid::API::Algorithm() {}
  /// Virtual destructor
  virtual ~InvertMDDim() {}
  /// Algorithm's name
  virtual const std::string name() const { return "InvertMDDim"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "MDAlgorithms"; }

private:
  /// Initialisation code
  void init();
  ///Execution code
  void exec();

  void copyMetaData( Mantid::API::IMDHistoWorkspace_sptr inws,  Mantid::API::IMDHistoWorkspace_sptr outws);
  void recurseDim(Mantid::API::IMDHistoWorkspace_sptr inWS, Mantid::API::IMDHistoWorkspace_sptr outWS,
  		  int currentDim, int *idx, int rank);

  unsigned int calcIndex(Mantid::API::IMDHistoWorkspace_sptr ws, int *dim);
  unsigned int calcInvertedIndex(Mantid::API::IMDHistoWorkspace_sptr ws, int *dim);

  virtual void initDocs();

};

#endif /*INVERTMDDIM_H_*/
