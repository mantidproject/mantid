/*WIKI* 


This algorithm can be useful when working with large datasets. It enables the raw
file to be loaded in two parts (not necessarily of equal size), the data processed
in turn and the results joined back together into a single dataset.
This can help avoid memory problems either because intermediate workspaces will
be smaller and/or because the data will be much reduced after processing.

The output of the algorithm, in which the data from the second input workspace will be appended to the first, will be stored under the name of the first input workspace.
Workspace data members other than the data (e.g. instrument etc.) will be copied
from the first input workspace (but if they're not identical anyway, then you probably
shouldn't be using this algorithm!). Both input workspaces will be deleted.

==== Restrictions on the input workspace ====

The input workspaces must come from the same instrument, have common units and bins
and no detectors that contribute to spectra should overlap.

==== Second Version ====

The [[ConjoinWorkspaces_v.2|second version of ConjoinWorkspaces]] supports having
a different OutputWorkspace than InputWorkspace1.


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ConjoinWorkspaces.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidGeometry/ISpectraDetectorMap.h"
#include "MantidAlgorithms/ConjoinWorkspaces2.h"

namespace Mantid
{
namespace Algorithms
{

using std::size_t;
using Geometry::ISpectraDetectorMap;
using namespace Kernel;
using namespace API;
using namespace DataObjects;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConjoinWorkspaces)

/// Sets documentation strings for this algorithm
void ConjoinWorkspaces::initDocs()
{
  this->setWikiSummary("Joins two partial, non-overlapping 2D workspaces into one. ");
  this->setOptionalMessage("Joins two partial, non-overlapping 2D workspaces into one.");
}


//----------------------------------------------------------------------------------------------
/// Default constructor
ConjoinWorkspaces::ConjoinWorkspaces() : Algorithm()
{
}

//----------------------------------------------------------------------------------------------
/// Destructor
ConjoinWorkspaces::~ConjoinWorkspaces() 
{

}

//----------------------------------------------------------------------------------------------
/** Initialize the properties */
void ConjoinWorkspaces::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace1",
    "", Direction::InOut, new CommonBinsValidator<>),
    "The name of the first input workspace");
  declareProperty(new WorkspaceProperty<>("InputWorkspace2",
    "", Direction::Input, new CommonBinsValidator<>),
    "The name of the second input workspace");
  declareProperty(new PropertyWithValue<bool>("CheckOverlapping", true, Direction::Input),
                  "Verify that the supplied data do not overlap");
}

//----------------------------------------------------------------------------------------------
/** Executes the algorithm
 *  @throw std::invalid_argument If the input workspaces do not meet the requirements of this algorithm
 */
void ConjoinWorkspaces::exec()
{

  //Call ConjoinWorkspaces2
  IAlgorithm_sptr alg = this->createSubAlgorithm("ConjoinWorkspaces", 0, 1.0, true, 2);
  alg->initialize();
  alg->setPropertyValue("InputWorkspace1", this->getPropertyValue("InputWorkspace1"));
  alg->setPropertyValue("InputWorkspace2", this->getPropertyValue("InputWorkspace2"));
  alg->setPropertyValue("CheckOverlapping", this->getPropertyValue("CheckOverlapping"));
  // Output is the InputWorkspace1
  alg->setPropertyValue("OutputWorkspace", this->getPropertyValue("InputWorkspace1"));
  alg->execute();
  if (!alg->isExecuted())
    throw std::runtime_error("Error running ConjoinWorkspaces2. See log.");

  MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");

  // Delete the input workspaces from the ADS
  AnalysisDataService::Instance().remove(getPropertyValue("InputWorkspace1"));
  AnalysisDataService::Instance().remove(getPropertyValue("InputWorkspace2"));

  // The InputWorkspace1 property in the input
  this->setProperty("InputWorkspace1", outWS);

}


} // namespace Algorithm
} // namespace Mantid
