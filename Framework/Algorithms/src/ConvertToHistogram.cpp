//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAlgorithms/ConvertToHistogram.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/VectorHelper.h"

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(ConvertToHistogram)

using API::MatrixWorkspace_sptr;
using Mantid::MantidVec;
using std::size_t;

//------------------------------------------------------------------------------
// Private member functions
//------------------------------------------------------------------------------

/**
 * Returns true if the algorithm needs to be run.
 * @param inputWS pointer to input workspace
 * @returns True if the input workspace needs to be run through this algorithm
 */
bool ConvertToHistogram::isProcessingRequired(
    const MatrixWorkspace_sptr inputWS) const {
  if (inputWS->isHistogramData()) {
    g_log.information() << "Input workspace already contains histogram data. "
                        << "OutputWorkspace set to InputWorkspace value.\n";
    return false;
  }
  return true;
}

/**
 * Returns the size of the new X vector
 * @param ySize pointer to input workspace
 * @returns An integer giving the size of the new X vector
 */
size_t ConvertToHistogram::getNewXSize(const std::size_t ySize) const {
  return ySize + 1;
}

/**
 * Calculate the histogram boundaries. For uniform bins this should work
 * correctly
 * and should be convertable back to point data. For non-uniform bins the
 * boundaries
 * are guessed such that the boundary goes mid-way between each point
 * @param inputX :: A const reference to the input data
 */
Kernel::cow_ptr<HistogramData::HistogramX> ConvertToHistogram::calculateXPoints(
    Kernel::cow_ptr<HistogramData::HistogramX> inputX) const {
  return HistogramData::BinEdges(HistogramData::Points(std::move(inputX)))
      .cowData();
}
} // namespace Algorithms
} // namespace Mantid
