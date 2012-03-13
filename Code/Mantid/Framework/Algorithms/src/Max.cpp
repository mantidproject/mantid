/*WIKI* 




*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Max.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidAPI/Progress.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(Max)

using namespace Kernel;
using namespace API;

/// Set the documentation strings
void Max::initDocs()
{
  this->setWikiSummary("Takes a 2D workspace as input and find the maximum in each 1D spectrum. The algorithm creates a new 1D workspace containing all maxima as well as their X boundaries and error. This is used in particular for single crystal as a quick way to find strong peaks.");
  this->setOptionalMessage("Takes a 2D workspace as input and find the maximum in each 1D spectrum. The algorithm creates a new 1D workspace containing all maxima as well as their X boundaries and error. This is used in particular for single crystal as a quick way to find strong peaks.");
}

/** Initialisation method.
 *
 */
void Max::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,boost::make_shared<HistogramValidator>()),
      "The name of the Workspace2D to take as input");
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
      "The name of the workspace in which to store the result");

  declareProperty("RangeLower",EMPTY_DBL(),
      "The X value to search from (default min)");
  declareProperty("RangeUpper",EMPTY_DBL(),
      "The X value to search to (default max)");
  auto mustBePositive = boost::make_shared<BoundedValidator<int> >();
  mustBePositive->setLower(0);
  declareProperty("StartWorkspaceIndex",0, mustBePositive,"Start spectrum number (default 0)");
  declareProperty("EndWorkspaceIndex",EMPTY_INT(), mustBePositive, "End spectrum number  (default max)");
}

/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void Max::exec()
{
  // Try and retrieve the optional properties
  double m_MinRange = getProperty("RangeLower");
  double m_MaxRange = getProperty("RangeUpper");
  int m_MinSpec = getProperty("StartWorkspaceIndex");
  int m_MaxSpec = getProperty("EndWorkspaceIndex");


  // Get the input workspace
  MatrixWorkspace_sptr inworkspace = getProperty("InputWorkspace");


  // sub-algorithme does all of the actual work - do not set the output workspace
  IAlgorithm_sptr maxAlgo = createSubAlgorithm("MaxMin", 0., 1.);
  maxAlgo->setProperty("InputWorkspace", inworkspace);
  maxAlgo->setProperty("RangeLower", m_MinRange);
  maxAlgo->setProperty("RangeUpper", m_MaxRange);
  maxAlgo->setProperty("StartWorkspaceIndex", m_MinSpec);
  maxAlgo->setProperty("EndWorkspaceIndex", m_MaxSpec);
  maxAlgo->setProperty("ShowMin",false);
  maxAlgo->execute();
  // just grab the child's output workspace
  MatrixWorkspace_sptr outputWS = maxAlgo->getProperty("OutputWorkspace");

  this->setProperty("OutputWorkspace", outputWS);
}

} // namespace Algorithms
} // namespace Mantid
