// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/EventWorkspaceHelpers.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Exception.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid::DataObjects {

/** Converts an EventWorkspace to an equivalent Workspace2D
 * @param inputMatrixW :: input event workspace
 * @return a MatrixWorkspace_sptr
 */
MatrixWorkspace_sptr EventWorkspaceHelpers::convertEventTo2D(const MatrixWorkspace_sptr &inputMatrixW) {
  EventWorkspace_sptr inputW = std::dynamic_pointer_cast<EventWorkspace>(inputMatrixW);
  if (!inputW)
    throw std::invalid_argument("EventWorkspaceHelpers::convertEventTo2D(): "
                                "Input workspace is not an EventWorkspace.");

  size_t numBins = inputW->blocksize();

  // Make a workspace 2D version of it
  MatrixWorkspace_sptr outputW;
  outputW = WorkspaceFactory::Instance().create("Workspace2D", inputW->getNumberHistograms(), numBins + 1, numBins);
  WorkspaceFactory::Instance().initializeFromParent(*inputW, *outputW, false);

  // Now let's set all the X bins and values
  for (size_t i = 0; i < inputW->getNumberHistograms(); i++) {
    outputW->getSpectrum(i).copyInfoFrom(inputW->getSpectrum(i));
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

} // namespace Mantid::DataObjects
