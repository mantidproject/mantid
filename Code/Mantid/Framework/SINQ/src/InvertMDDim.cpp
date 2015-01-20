/**
 * This Algorithms inverts the dimensions of a MD data set. The
 * application area is when fixing up MD workspaces which had to have the
 * dimensions inverted because they were delivered in C storage order.
 *
 * copyright: leave me alone or mantid Copyright
 *
 * Mark Koennecke, Dezember 2012
 */
#include "MantidSINQ/InvertMDDim.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(InvertMDDim)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::MDEvents;
using namespace Mantid;

// A reference to the logger is provided by the base class, it is called g_log.
// It is used to print out information, warning and error messages

void InvertMDDim::init() {
  declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("InputWorkspace", "",
                                                           Direction::Input));
  declareProperty(new WorkspaceProperty<IMDHistoWorkspace>(
      "OutputWorkspace", "", Direction::Output));
}

void InvertMDDim::exec() {
  IMDHistoWorkspace_sptr inWS =
      IMDHistoWorkspace_sptr(getProperty("InputWorkspace"));
  std::vector<IMDDimension_sptr> dimensions;
  for (int i = static_cast<int>(inWS->getNumDims()) - 1; i >= 0; i--) {
    boost::shared_ptr<const IMDDimension> dimi = inWS->getDimension(i);
    dimensions.push_back(boost::const_pointer_cast<IMDDimension>(dimi));
  }

  MDHistoWorkspace_sptr outWS(new MDHistoWorkspace(dimensions));
  outWS->setTo(.0, .0, .0);

  int rank = static_cast<int>(inWS->getNumDims());
  int *idx = new int[rank];
  if (idx == NULL || outWS == NULL) {
    throw std::runtime_error("Out of memory in InvertMDDim");
  }
  recurseDim(inWS, outWS, 0, idx, rank);
  delete[] idx;

  copyMetaData(inWS, outWS);

  // assign the workspace
  setProperty("OutputWorkspace", outWS);
}

void InvertMDDim::recurseDim(IMDHistoWorkspace_sptr inWS,
                             IMDHistoWorkspace_sptr outWS, int currentDim,
                             int *idx, int rank) {
  boost::shared_ptr<const IMDDimension> dimi = inWS->getDimension(currentDim);
  if (currentDim == rank - 1) {
    for (int i = 0; i < static_cast<int>(dimi->getNBins()); i++) {
      idx[currentDim] = i;
      unsigned int inIDX = calcIndex(inWS, idx);
      unsigned int outIDX = calcInvertedIndex(outWS, idx);
      outWS->setSignalAt(outIDX, inWS->signalAt(inIDX));
      outWS->setErrorSquaredAt(outIDX, inWS->errorSquaredAt(inIDX));
    }
  } else {
    for (int i = 0; i < static_cast<int>(dimi->getNBins()); i++) {
      idx[currentDim] = i;
      recurseDim(inWS, outWS, currentDim + 1, idx, rank);
    }
  }
}

void InvertMDDim::copyMetaData(Mantid::API::IMDHistoWorkspace_sptr inws,
                               Mantid::API::IMDHistoWorkspace_sptr outws) {
  outws->setTitle(inws->getTitle());
  ExperimentInfo_sptr info;

  if (inws->getNumExperimentInfo() > 0) {
    info = inws->getExperimentInfo(0);
    outws->addExperimentInfo(info);
  }
}
/**
 * This is here is mostly a workaround for a missing feature in
 * IMDHistoWorkspace.
 * I.e. a proper address calculation from an index array.
 */
unsigned int InvertMDDim::calcIndex(IMDHistoWorkspace_sptr ws, int dim[]) {
  size_t idx = 0;
  switch (ws->getNumDims()) {
  case 2:
    idx = ws->getLinearIndex(dim[0], dim[1]);
    break;
  case 3:
    idx = ws->getLinearIndex(dim[0], dim[1], dim[2]);
    break;
  case 4:
    idx = ws->getLinearIndex(dim[0], dim[1], dim[2], dim[3]);
    break;
  default:
    throw std::runtime_error("Unsupported dimension depth");
  }
  return static_cast<unsigned int>(idx);
}

unsigned int InvertMDDim::calcInvertedIndex(IMDHistoWorkspace_sptr ws,
                                            int dim[]) {
  size_t idx = 0;
  switch (ws->getNumDims()) {
  case 2:
    idx = ws->getLinearIndex(dim[1], dim[0]);
    break;
  case 3:
    idx = ws->getLinearIndex(dim[2], dim[1], dim[0]);
    break;
  case 4:
    idx = ws->getLinearIndex(dim[3], dim[2], dim[1], dim[0]);
    break;
  default:
    throw std::runtime_error("Unsupported dimension depth");
  }
  return static_cast<unsigned int>(idx);
}
