// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Min.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid::Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(Min)

using namespace Kernel;
using namespace API;

/** Initialisation method.
 *
 */
void Min::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input,
                                                        std::make_shared<HistogramValidator>()),
                  "The name of the Workspace2D to take as input");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "The name of the workspace in which to store the result");

  declareProperty("RangeLower", EMPTY_DBL(), "The X value to search from (default min)");
  declareProperty("RangeUpper", EMPTY_DBL(), "The X value to search to (default max)");
  auto mustBePositive = std::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty("StartWorkspaceIndex", 0, mustBePositive, "Start spectrum number (default 0)");
  declareProperty("EndWorkspaceIndex", EMPTY_INT(), mustBePositive, "End spectrum number  (default max)");
}

/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void Min::exec() {
  // Try and retrieve the optional properties
  double m_MinRange = getProperty("RangeLower");
  double m_MaxRange = getProperty("RangeUpper");
  int m_MinSpec = getProperty("StartWorkspaceIndex");
  int m_MaxSpec = getProperty("EndWorkspaceIndex");

  // Get the input workspace
  MatrixWorkspace_sptr inworkspace = getProperty("InputWorkspace");

  // Child Algorithme does all of the actual work - do not set the output
  // workspace
  auto minAlgo = createChildAlgorithm("MaxMin", 0., 1.);
  minAlgo->setProperty("InputWorkspace", inworkspace);
  minAlgo->setProperty("RangeLower", m_MinRange);
  minAlgo->setProperty("RangeUpper", m_MaxRange);
  minAlgo->setProperty("StartWorkspaceIndex", m_MinSpec);
  minAlgo->setProperty("EndWorkspaceIndex", m_MaxSpec);
  minAlgo->setProperty("ShowMin", true);
  minAlgo->execute();
  // just grab the child's output workspace
  MatrixWorkspace_sptr outputWS = minAlgo->getProperty("OutputWorkspace");

  this->setProperty("OutputWorkspace", outputWS);
}

} // namespace Mantid::Algorithms
