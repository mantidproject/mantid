/**
 * This algorithm takes a MDHistoWorkspace and allows to select a slab out of
 * it which is storeed into the result workspace.
 *
 * copyright: do not bother me or use mantid copyright
 *
 * Mark Koennecke, November 2012
 */
#ifndef SLICEMDHISTO_H_
#define SLICEMDHISTO_H_

#include "MantidSINQ/DllConfig.h"
#include "MantidAPI/Algorithm.h"
#include "MantidMDEvents/MDHistoWorkspace.h"

class MANTID_SINQ_DLL SliceMDHisto : public Mantid::API::Algorithm
{
public:
  /// (Empty) Constructor
  SliceMDHisto() : Mantid::API::Algorithm(), dim() {}
  /// Virtual destructor
  virtual ~SliceMDHisto() {}
  /// Algorithm's name
  virtual const std::string name() const { return "SliceMDHisto"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "MDAlgorithms"; }

private:
  /// Initialisation code
  void init();
  ///Execution code
  void exec();

  unsigned int rank;
  std::vector<int>dim;
  void cutData(Mantid::API::IMDHistoWorkspace_sptr inWS,
		  Mantid::API::IMDHistoWorkspace_sptr outWS,
		  Mantid::coord_t *sourceDim, Mantid::coord_t *targetDim,
		  std::vector<int> start, std::vector<int> end, unsigned int dim);

  void copyMetaData( Mantid::API::IMDHistoWorkspace_sptr inws,  Mantid::API::IMDHistoWorkspace_sptr outws);

  virtual void initDocs();

};

#endif /*SLICEMDHISTO_H_*/
