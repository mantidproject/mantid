//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/CropWorkspace.h"

#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/TextAxis.h"
#include "MantidKernel/VectorHelper.h"
#include <iostream>
#include "MantidAPI/MemoryManager.h"
#include "MantidKernel/BoundedValidator.h"

namespace {
/// The percentage 'fuzziness' to use when comparing to bin boundaries
const double xBoundaryTolerance = 1.0e-15;
}

namespace Mantid {
namespace Algorithms {

using std::size_t;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CropWorkspace)

using namespace Kernel;
using namespace API;

/// Default constructor
CropWorkspace::CropWorkspace(): Algorithm() {}

/// Destructor
CropWorkspace::~CropWorkspace() {}

void CropWorkspace::init() {
  declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
      "The input workspace");
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "Name of the output workspace");

  declareProperty("XMin", EMPTY_DBL(), "An X value that is within the first "
                                       "(lowest X value) bin that will be "
                                       "retained\n"
                                       "(default: workspace min)");
  declareProperty("XMax", EMPTY_DBL(), "An X value that is in the highest X "
                                       "value bin to be retained (default: max "
                                       "X)");
  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty("StartWorkspaceIndex", 0, mustBePositive,
                  "The index number of the first entry in the Workspace that "
                  "will be loaded\n"
                  "(default: first entry in the Workspace)");
  // As the property takes ownership of the validator pointer, have to take care
  // to pass in a unique
  // pointer to each property.
  declareProperty(
      "EndWorkspaceIndex", EMPTY_INT(), mustBePositive,
      "The index number of the last entry in the Workspace to be loaded\n"
      "(default: last entry in the Workspace)");
}

/** Executes the algorithm
 *  @throw std::out_of_range If a property is set to an invalid value for the
 * input workspace
 */
void CropWorkspace::exec() {

  auto extract = createChildAlgorithm("ExtractSpectra");
  extract->initialize();
  extract->copyPropertiesFrom(*this);
  extract->setRethrows(true);
  extract->execute();

  MatrixWorkspace_sptr outputWorkspace = extract->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace", outputWorkspace);
}


} // namespace Algorithms
} // namespace Mantid
