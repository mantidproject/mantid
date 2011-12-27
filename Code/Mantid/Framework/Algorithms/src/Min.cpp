/*WIKI* 




*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Min.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidAPI/Progress.h"

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(Min)

using namespace Kernel;
using namespace API;

/// Set the documentation strings
void Min::initDocs()
{
  this->setWikiSummary("Takes a 2D workspace as input and find the minimum in each 1D spectrum. The algorithm creates a new 1D workspace containing all minima as well as their X boundaries and error. This is used in particular for single crystal as a quick way to find strong peaks.");
  this->setOptionalMessage("Takes a 2D workspace as input and find the minimum in each 1D spectrum. The algorithm creates a new 1D workspace containing all minima as well as their X boundaries and error. This is used in particular for single crystal as a quick way to find strong peaks.");
}

/** Initialisation method.
 *
 */
void Min::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,new HistogramValidator<>),
      "The name of the Workspace2D to take as input");
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
      "The name of the workspace in which to store the result");

  declareProperty("RangeLower",EMPTY_DBL(),
      "The X value to search from (default min)");
  declareProperty("RangeUpper",EMPTY_DBL(),
      "The X value to search to (default max)");
  BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
  mustBePositive->setLower(0);
  declareProperty("StartWorkspaceIndex",0, mustBePositive,
      "Start spectrum number (default 0)");
  // As the property takes ownership of the validator pointer, have to take care to pass in a unique
  // pointer to each property.
  declareProperty("EndWorkspaceIndex",EMPTY_INT(), mustBePositive->clone(),
      "End spectrum number  (default max)");
}

/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void Min::exec()
{
  // Try and retrieve the optional properties
  double m_MinRange = getProperty("RangeLower");
  double m_MaxRange = getProperty("RangeUpper");
  int m_MinSpec = getProperty("StartWorkspaceIndex");
  int m_MaxSpec = getProperty("EndWorkspaceIndex");


  // Get the input workspace
  MatrixWorkspace_sptr inworkspace = getProperty("InputWorkspace");


  // sub-algorithme does all of the actual work - do not set the output workspace
  IAlgorithm_sptr minAlgo = createSubAlgorithm("MaxMin", 0., 1.);
  minAlgo->setProperty("InputWorkspace", inworkspace);
  minAlgo->setProperty("RangeLower", m_MinRange);
  minAlgo->setProperty("RangeUpper", m_MaxRange);
  minAlgo->setProperty("StartWorkspaceIndex", m_MinSpec);
  minAlgo->setProperty("EndWorkspaceIndex", m_MaxSpec);
  minAlgo->setProperty("ShowMin",true);
  minAlgo->execute();
  // just grab the child's output workspace
  MatrixWorkspace_sptr outputWS = minAlgo->getProperty("OutputWorkspace");

  this->setProperty("OutputWorkspace", outputWS);

}

} // namespace Algorithms
} // namespace Mantid
