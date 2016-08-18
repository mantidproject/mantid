//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAlgorithms/ConvertToPointData.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/VectorHelper.h"

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(ConvertToPointData)

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
bool ConvertToPointData::isProcessingRequired(
    const MatrixWorkspace_sptr inputWS) const {
  if (!inputWS->isHistogramData()) {
    g_log.information() << "Input workspace already contains point data. "
                        << "OutputWorkspace set to InputWorkspace value.\n";
    return false;
  }
  return true;
}

/**
 * Returns the size of the new X vector
 * @param inputWS pointer to input workspace
 * @returns An integer giving the size of the new X vector
 */
size_t
ConvertToPointData::getNewXSize(const MatrixWorkspace_sptr inputWS) const {
  return static_cast<int>(inputWS->blocksize());
}

/**
 * Calculate the X point values
 * @param inputX :: A const reference to the input data
 * @param outputX :: A reference to the output data
 */
void ConvertToPointData::calculateXPoints(const MantidVec &inputX,
                                          MantidVec &outputX) const {
  Kernel::VectorHelper::convertToBinCentre(inputX, outputX);
}
}
}
