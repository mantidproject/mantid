// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/**
 * This algorithm takes a MDHistoWorkspace and allows to select a slab out of
 * it which is stored into the result workspace.
 *
 * Mark Koennecke, November 2012
 */
#include "MantidSINQ/SliceMDHisto.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/ArrayProperty.h"

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SliceMDHisto)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using namespace Mantid;

// A reference to the logger is provided by the base class, it is called g_log.
// It is used to print out information, warning and error messages

SliceMDHisto::SliceMDHisto() : Mantid::API::Algorithm(), m_rank(0), m_dim() {}

void SliceMDHisto::init() {
  declareProperty(std::make_unique<WorkspaceProperty<IMDHistoWorkspace>>("InputWorkspace", "", Direction::Input));
  declareProperty(std::make_unique<ArrayProperty<int>>("Start"), "A comma separated list of min,for each dimension");
  declareProperty(std::make_unique<ArrayProperty<int>>("End"), "A comma separated list of max for each dimension");
  declareProperty(std::make_unique<WorkspaceProperty<IMDHistoWorkspace>>("OutputWorkspace", "", Direction::Output));
}

void SliceMDHisto::exec() {
  IMDHistoWorkspace_sptr inWS = IMDHistoWorkspace_sptr(getProperty("InputWorkspace"));
  m_rank = static_cast<unsigned int>(inWS->getNumDims());
  for (unsigned int i = 0; i < m_rank; i++) {
    std::shared_ptr<const IMDDimension> arDim = inWS->getDimension(i);
    m_dim.emplace_back(static_cast<int>(arDim->getNBins()));
  }

  std::vector<int> start = getProperty("Start");
  std::vector<int> end = getProperty("End");

  // some checks
  if (start.size() < m_rank || end.size() < m_rank) {
    throw std::runtime_error("Start and end need to be given for each dimension of the dataset");
  }
  for (unsigned int i = 0; i < m_rank; i++) {
    if (start[i] < 0) {
      start[i] = 0;
    }
    if (start[i] > m_dim[i]) {
      start[i] = m_dim[i];
    }
    if (end[i] < 0) {
      end[i] = 0;
    }
    if (end[i] >= m_dim[i]) {
      end[i] = m_dim[i];
    }
    if (end[i] < start[i]) {
      throw std::runtime_error("End must be larger then start for each dimension");
    }
  }

  // create the new dadaset
  std::vector<MDHistoDimension_sptr> dimensions;
  for (unsigned int k = 0; k < m_rank; ++k) {
    std::shared_ptr<const IMDDimension> arDim = inWS->getDimension(k);
    dimensions.emplace_back(
        MDHistoDimension_sptr(new MDHistoDimension(arDim->getName(), arDim->getName(), arDim->getMDFrame(),
                                                   arDim->getX(start[k]), arDim->getX(end[k]), end[k] - start[k])));
  }
  auto outWS = std::make_shared<MDHistoWorkspace>(dimensions);

  auto *sourceDim = reinterpret_cast<coord_t *>(malloc(m_rank * sizeof(coord_t)));
  auto *targetDim = reinterpret_cast<coord_t *>(malloc(m_rank * sizeof(coord_t)));
  cutData(inWS, outWS, sourceDim, targetDim, start, end, 0);
  free(sourceDim);
  free(targetDim);

  // assign the workspace
  copyMetaData(inWS, outWS);
  setProperty("OutputWorkspace", outWS);
}

void SliceMDHisto::cutData(const Mantid::API::IMDHistoWorkspace_sptr &inWS,
                           const Mantid::API::IMDHistoWorkspace_sptr &outWS, Mantid::coord_t *sourceDim,
                           Mantid::coord_t *targetDim, std::vector<int> &start, std::vector<int> &end,
                           unsigned int dim) {
  int length;

  std::shared_ptr<const IMDDimension> inDim = inWS->getDimension(dim);
  std::shared_ptr<const IMDDimension> outDim = outWS->getDimension(dim);
  length = end[dim] - start[dim];
  if (dim == m_rank - 1) {
    MDHistoWorkspace_sptr outWSS = std::dynamic_pointer_cast<MDHistoWorkspace>(outWS);
    for (int i = 0; i < length; i++) {
      sourceDim[dim] = inDim->getX(start[dim] + i);
      signal_t val = inWS->getSignalAtCoord(sourceDim, static_cast<Mantid::API::MDNormalization>(0));
      targetDim[dim] = outDim->getX(i);
      size_t idx = outWSS->getLinearIndexAtCoord(targetDim);
      outWS->setSignalAt(idx, val);
      outWS->setErrorSquaredAt(idx, val);
    }
  } else {
    for (int i = 0; i < length; i++) {
      sourceDim[dim] = inDim->getX(start[dim] + i);
      targetDim[dim] = outDim->getX(i);
      cutData(inWS, outWS, sourceDim, targetDim, start, end, dim + 1);
    }
  }
}

void SliceMDHisto::copyMetaData(const Mantid::API::IMDHistoWorkspace_sptr &inws,
                                const Mantid::API::IMDHistoWorkspace_sptr &outws) {
  outws->setTitle(inws->getTitle());
  ExperimentInfo_sptr info;

  if (inws->getNumExperimentInfo() > 0) {
    info = inws->getExperimentInfo(0);
    outws->addExperimentInfo(info);
  }
}
