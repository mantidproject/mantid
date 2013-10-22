/**
 * This algorithm flattens a MDHistoWorkspace to a Workspace2D. Mantid has far more tools
 * to deal with W2D then for MD ones.
 *
 * copyright: do not bother me, see Mantid copyright
 *
 * Mark Koennecke, November 2012
 */
#ifndef MDHISTOTOWORKSPACE2D_H_
#define MDHISTOTOWORKSPACE2D_H_

#include "MantidSINQ/DllConfig.h"
#include "MantidAPI/Algorithm.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidDataObjects/Workspace2D.h"

class MANTID_SINQ_DLL MDHistoToWorkspace2D : public Mantid::API::Algorithm
{
public:
  /// (Empty) Constructor
  MDHistoToWorkspace2D() : Mantid::API::Algorithm() {}
  /// Virtual destructor
  virtual ~MDHistoToWorkspace2D() {}
  /// Algorithm's name
  virtual const std::string name() const { return "MDHistoToWorkspace2D"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "MDAlgorithms"; }

private:
  /// Initialisation code
  void init();
  ///Execution code
  void exec();

  virtual void initDocs();

  size_t rank;
  size_t currentSpectra;
  size_t calculateNSpectra( Mantid::API::IMDHistoWorkspace_sptr inws);
  void recurseData(Mantid::API::IMDHistoWorkspace_sptr inWS, Mantid::DataObjects::Workspace2D_sptr outWS, size_t currentDim, Mantid::coord_t *pos);

  void checkW2D(Mantid::DataObjects::Workspace2D_sptr outWS);

  void copyMetaData(Mantid::API::IMDHistoWorkspace_sptr inWS, Mantid::DataObjects::Workspace2D_sptr outWS);
};

#endif /*MDHISTOTOWORKSPACE2D_H_*/
