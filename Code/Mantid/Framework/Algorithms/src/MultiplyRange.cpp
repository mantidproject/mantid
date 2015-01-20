//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/MultiplyRange.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid {
namespace Algorithms {

// Algorithm must be declared
DECLARE_ALGORITHM(MultiplyRange)

using namespace Kernel;
using namespace API;

void MultiplyRange::init() {
  // Declare an input workspace property.
  declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
      "The name of the input workspace.");
  // Declare an output workspace property.
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
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
  m_startBin = getProperty("StartBin");
  m_endBin = getProperty("EndBin");
  m_factor = getProperty("Factor");

  // A few checks on the input properties
  const int specSize = static_cast<int>(inputWS->blocksize());
  if (isEmpty(m_endBin))
    m_endBin = specSize - 1;

  if (m_endBin >= specSize) {
    g_log.error("EndBin out of range!");
    throw std::out_of_range("EndBin out of range!");
  }
  if (m_endBin < m_startBin) {
    g_log.error("StartBin must be less than or equal to EndBin");
    throw std::out_of_range("StartBin must be less than or equal to EndBin");
  }

  // Only create the output workspace if it's different to the input one
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if (outputWS != inputWS) {
    outputWS = WorkspaceFactory::Instance().create(inputWS);
    setProperty("OutputWorkspace", outputWS);
  }

  // Get the count of histograms in the input workspace
  const int histogramCount = static_cast<int>(inputWS->getNumberHistograms());
  Progress progress(this, 0.0, 1.0, histogramCount);
  // Loop over spectra
  PARALLEL_FOR2(inputWS, outputWS)
  for (int i = 0; i < histogramCount; ++i) {
    PARALLEL_START_INTERUPT_REGION
    // Copy over the bin boundaries
    outputWS->setX(i, inputWS->refX(i));
    // Copy over the data
    outputWS->dataY(i) = inputWS->readY(i);
    outputWS->dataE(i) = inputWS->readE(i);
    MantidVec &newY = outputWS->dataY(i);
    MantidVec &newE = outputWS->dataE(i);

    // Now multiply the requested range
    std::transform(newY.begin() + m_startBin, newY.begin() + m_endBin + 1,
                   newY.begin() + m_startBin,
                   std::bind2nd(std::multiplies<double>(), m_factor));
    std::transform(newE.begin() + m_startBin, newE.begin() + m_endBin + 1,
                   newE.begin() + m_startBin,
                   std::bind2nd(std::multiplies<double>(), m_factor));

    progress.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
}

} // namespace Algorithms
} // namespace Mantid
