// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/MultiplyRange.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid {
namespace Algorithms {

// Algorithm must be declared
DECLARE_ALGORITHM(MultiplyRange)

using namespace Kernel;
using namespace API;
using namespace DataObjects;
using namespace HistogramData;

void MultiplyRange::init() {
  // Declare an input workspace property.
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "",
                                                        Direction::Input),
                  "The name of the input workspace.");
  // Declare an output workspace property.
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                        Direction::Output),
                  "The name of the output workspace.");

  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  // StartBin
  declareProperty("StartBin", 0, mustBePositive, "Bin index to start from");
  // EndBin
  declareProperty("EndBin", EMPTY_INT(), mustBePositive,
                  "Bin index to finish at");
  // factor
  declareProperty("Factor", 0.0,
                  "The value by which to multiply the input data range");
}

/** Executes the algorithm
 */
void MultiplyRange::exec() {
  // Get the input workspace and other properties
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  int startBin = getProperty("StartBin");
  int endBin = getProperty("EndBin");
  double factor = getProperty("Factor");

  // A few checks on the input properties
  const int specSize = static_cast<int>(inputWS->blocksize());
  if (isEmpty(endBin))
    endBin = specSize - 1;

  if (endBin >= specSize) {
    g_log.error("EndBin out of range!");
    throw std::out_of_range("EndBin out of range!");
  }
  if (endBin < startBin) {
    g_log.error("StartBin must be less than or equal to EndBin");
    throw std::out_of_range("StartBin must be less than or equal to EndBin");
  }

  // Only create the output workspace if it's different to the input one
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if (outputWS != inputWS) {
    outputWS = create<MatrixWorkspace>(*inputWS);
    setProperty("OutputWorkspace", outputWS);
  }

  // Get the count of histograms in the input workspace
  const int histogramCount = static_cast<int>(inputWS->getNumberHistograms());
  Progress progress(this, 0.0, 1.0, histogramCount);
  // Loop over spectra
  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *outputWS))
  for (int i = 0; i < histogramCount; ++i) {
    PARALLEL_START_INTERUPT_REGION
    outputWS->setHistogram(i, inputWS->histogram(i));
    auto &newY = outputWS->mutableY(i);
    auto &newE = outputWS->mutableE(i);

    // Now multiply the requested range
    std::transform(newY.begin() + startBin, newY.begin() + endBin + 1,
                   newY.begin() + startBin,
                   std::bind2nd(std::multiplies<double>(), factor));
    std::transform(newE.begin() + startBin, newE.begin() + endBin + 1,
                   newE.begin() + startBin,
                   std::bind2nd(std::multiplies<double>(), factor));

    progress.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
}

} // namespace Algorithms
} // namespace Mantid
