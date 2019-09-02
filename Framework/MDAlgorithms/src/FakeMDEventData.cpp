// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------------------------
#include "MantidMDAlgorithms/FakeMDEventData.h"

#include "MantidDataObjects/FakeMD.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid {
namespace MDAlgorithms {

using namespace API;
using namespace DataObjects;
using namespace Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FakeMDEventData)

/** Initialize the algorithm's properties.
 */
void FakeMDEventData::init() {
  declareProperty(std::make_unique<WorkspaceProperty<IMDEventWorkspace>>(
                      "InputWorkspace", "", Direction::InOut),
                  "An input workspace, that will get DataObjects added to it");

  declareProperty(std::make_unique<ArrayProperty<double>>("UniformParams", ""),
                  "Add a uniform, randomized distribution of events.\n"
                  "1 parameter: number_of_events; they will be distributed "
                  "across the size of the workspace.\n"
                  "Depending on the sign of this parameter, the events are "
                  "either distributed randomly around the box \n"
                  "(Case 1, positive) or placed on the regular grid through "
                  "the box (Case 2, negative)\n"
                  "Treatment of Multiple parameters: depends on the Case\n"
                  "Case 1: number_of_events, min,max (for each dimension); "
                  "distribute the events inside the range given.\n"
                  "Case 2: Additional parameters describe initial location and "
                  "steps of the regular grid in each dimension\n");

  declareProperty(
      std::make_unique<ArrayProperty<double>>("PeakParams", ""),
      "Add a peak with a normal distribution around a central point.\n"
      "Parameters: number_of_events, x, y, z, ..., radius.\n");

  declareProperty(std::make_unique<PropertyWithValue<int>>("RandomSeed", 0),
                  "Seed int for the random number generator.");

  declareProperty(
      std::make_unique<PropertyWithValue<bool>>("RandomizeSignal", false),
      "If true, the events' signal and error values will be "
      "randomized around 1.0+-0.5.");
}

/**
 * Execute the algorithm.
 */
void FakeMDEventData::exec() {

  FakeMD faker(getProperty("UniformParams"), getProperty("PeakParams"),
               getProperty("RandomSeed"), getProperty("RandomizeSignal"));
  faker.fill(getProperty("InputWorkspace"));
}

} // namespace MDAlgorithms
} // namespace Mantid
