// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CropWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid {
namespace Algorithms {

using std::size_t;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CropWorkspace)

using namespace Kernel;
using namespace API;

void CropWorkspace::init() {
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
      "The input workspace");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "Name of the output workspace");

  declareProperty("XMin", EMPTY_DBL(),
                  "An X value that is within the first "
                  "(lowest X value) bin that will be "
                  "retained\n"
                  "(default: workspace min)");
  declareProperty("XMax", EMPTY_DBL(),
                  "An X value that is in the highest X "
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

  auto extract = createChildAlgorithm("ExtractSpectra", 0, 1);
  extract->initialize();
  extract->setRethrows(true);

  MatrixWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");
  extract->setProperty("InputWorkspace", inputWorkspace);

  MatrixWorkspace_sptr outputWorkspace = getProperty("OutputWorkspace");
  extract->setProperty("OutputWorkspace", outputWorkspace);

  double xmin = getProperty("XMin");
  extract->setProperty("XMin", xmin);

  double xmax = getProperty("XMax");
  extract->setProperty("XMax", xmax);

  int start = getProperty("StartWorkspaceIndex");
  extract->setProperty("StartWorkspaceIndex", start);

  int end = getProperty("EndWorkspaceIndex");
  extract->setProperty("EndWorkspaceIndex", end);

  extract->execute();

  outputWorkspace = extract->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace", outputWorkspace);
}

} // namespace Algorithms
} // namespace Mantid
