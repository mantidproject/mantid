#include "MantidDataObjects/EventWorkspaceHelpers.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/WorkspaceFactory.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid {
namespace DataObjects {

/** Converts an EventWorkspace to an equivalent Workspace2D
 * @param inputMatrixW :: input event workspace
 * @return a MatrixWorkspace_sptr
 */
MatrixWorkspace_sptr
EventWorkspaceHelpers::convertEventTo2D(MatrixWorkspace_sptr inputMatrixW) {
  EventWorkspace_sptr inputW =
      boost::dynamic_pointer_cast<EventWorkspace>(inputMatrixW);
  if (!inputW)
    throw std::invalid_argument("EventWorkspaceHelpers::convertEventTo2D(): "
                                "Input workspace is not an EventWorkspace.");

  size_t numBins = inputW->blocksize();

  // Make a workspace 2D version of it
  MatrixWorkspace_sptr outputW;
  outputW = WorkspaceFactory::Instance().create(
      "Workspace2D", inputW->getNumberHistograms(), numBins + 1, numBins);
  WorkspaceFactory::Instance().initializeFromParent(inputW, outputW, false);

  // Now let's set all the X bins and values
  for (size_t i = 0; i < inputW->getNumberHistograms(); i++) {
    outputW->getSpectrum(i)->copyInfoFrom(*inputW->getSpectrum(i));
    outputW->setX(i, inputW->refX(i));

    MantidVec &Yout = outputW->dataY(i);
    const MantidVec &Yin = inputW->readY(i);
    for (size_t j = 0; j < numBins; j++)
      Yout[j] = Yin[j];

    MantidVec &Eout = outputW->dataE(i);
    const MantidVec &Ein = inputW->readE(i);
    for (size_t j = 0; j < numBins; j++)
      Eout[j] = Ein[j];
  }

  return outputW;
}

} // namespace Mantid
} // namespace DataObjects
