/**
 * This is a little algorithm which can sum a MD dataset along one direction,
 * thereby yielding a dataset with one dimension less.
 *
 * copyright: do not bother me or my employer
 *
 * Mark Koennecke, November 2012
 */
#ifndef PROJECTMD_H_
#define PROJECTMD_H_

#include "MantidAPI/Algorithm.h"
#include "MantidMDEvents/MDHistoWorkspace.h"

class ProjectMD : public Mantid::API::Algorithm
{
public:
  /// (Empty) Constructor
  ProjectMD() : Mantid::API::Algorithm() {}
  /// Virtual destructor
  virtual ~ProjectMD() {}
  /// Algorithm's name
  virtual const std::string name() const { return "ProjectMD"; }
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
  void sumData( Mantid::API::IMDHistoWorkspace_sptr inws,  Mantid::API::IMDHistoWorkspace_sptr outws,
		  int *sourceDim, int *targetDim, int targetDimCount, int dimNo, int start, int end, int currentDim);

  double getValue(Mantid::API::IMDHistoWorkspace_sptr ws, int *dim);
  void putValue(Mantid::API::IMDHistoWorkspace_sptr ws, int *dim, double val);
  unsigned int calcIndex(Mantid::API::IMDHistoWorkspace_sptr ws, int *dim);

};

#endif /*PROJECTMD_H_*/
