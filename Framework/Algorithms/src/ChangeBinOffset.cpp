//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ChangeBinOffset.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;
using namespace DataObjects;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(ChangeBinOffset)

/**
 * Default constructor
 */
ChangeBinOffset::ChangeBinOffset()
    : API::Algorithm(), m_progress(nullptr), offset(0.), wi_min(0), wi_max(0) {}

/**
 * Destructor
 */
ChangeBinOffset::~ChangeBinOffset() {
  if (m_progress) {
    delete m_progress;
  }
}

/** Initialisation method. Declares properties to be used in algorithm.
*
*/
void ChangeBinOffset::init() {
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "",
                                                         Direction::Input),
                  "Name of the input workspace");
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "",
                                                         Direction::Output),
                  "Name of the output workspace");
  auto isDouble = boost::make_shared<BoundedValidator<double>>();
  declareProperty("Offset", 0.0, isDouble,
                  "The amount to change each time bin by");

  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty(
      "IndexMin", 0, mustBePositive,
      "The workspace index of the first spectrum to shift. Only used if\n"
      "IndexMax is set.");
  declareProperty(
      "IndexMax", Mantid::EMPTY_INT(), mustBePositive,
      "The workspace index of the last spectrum to shift. Only used if\n"
      "explicitly set.");
}

/** Executes the algorithm
 *
 */
void ChangeBinOffset::exec() {
  // Get input workspace and offset
  const MatrixWorkspace_sptr inputW = getProperty("InputWorkspace");

  offset = getProperty("Offset");

  // Get number of histograms
  int64_t histnumber = static_cast<int64_t>(inputW->getNumberHistograms());

  m_progress = new API::Progress(this, 0.0, 1.0, histnumber);

  wi_min = 0;
  wi_max = histnumber - 1;
  // check if workspace indexes have been set
  int tempwi_min = getProperty("IndexMin");
  int tempwi_max = getProperty("IndexMax");
  if (tempwi_max != Mantid::EMPTY_INT()) {
    // check wimin<=tempwi_min<=tempwi_max<=wi_max
    if ((wi_min <= tempwi_min) && (tempwi_min <= tempwi_max) &&
        (tempwi_max <= wi_max)) {
      wi_min = size_t(tempwi_min);
      wi_max = size_t(tempwi_max);
    } else {
      g_log.error("Invalid Workspace Index min/max properties");
      throw std::invalid_argument("Inconsistent properties defined");
    }
  }

  MatrixWorkspace_sptr outputW = getProperty("OutputWorkspace");
  if (outputW != inputW) {
    outputW = MatrixWorkspace_sptr(inputW->clone().release());
    setProperty("OutputWorkspace", outputW);
  }

  // Check if its an event workspace
  EventWorkspace_const_sptr eventWS =
      boost::dynamic_pointer_cast<const EventWorkspace>(inputW);
  if (eventWS != nullptr) {
    this->execEvent();
    return;
  }

  // do the shift in X
  PARALLEL_FOR1(outputW)
  for (int64_t i = wi_min; i <= wi_max; ++i) {
    PARALLEL_START_INTERUPT_REGION
    for (auto &x : outputW->dataX(i))
      x += offset;
    m_progress->report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // Copy units
  if (outputW->getAxis(0)->unit().get())
    outputW->getAxis(0)->unit() = inputW->getAxis(0)->unit();
  try {
    if (inputW->getAxis(1)->unit().get())
      outputW->getAxis(1)->unit() = inputW->getAxis(1)->unit();
  } catch (Exception::IndexError &) {
    // OK, so this isn't a Workspace2D
  }
}

void ChangeBinOffset::execEvent() {
  g_log.information("Processing event workspace");

  MatrixWorkspace_sptr matrixOutputWS = getProperty("OutputWorkspace");
  auto outputWS = boost::dynamic_pointer_cast<EventWorkspace>(matrixOutputWS);

  int64_t numHistograms = static_cast<int64_t>(outputWS->getNumberHistograms());
  PARALLEL_FOR1(outputWS)
  for (int64_t i = 0; i < numHistograms; ++i) {
    PARALLEL_START_INTERUPT_REGION
    // Do the offsetting
    if ((i >= wi_min) && (i <= wi_max))
      outputWS->getEventList(i).addTof(offset);
    m_progress->report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  outputWS->clearMRU();
}

} // namespace Algorithm
} // namespace Mantid
