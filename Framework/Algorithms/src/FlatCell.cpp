// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/FlatCell.h"
#include "MantidAPI/Algorithm.hxx"
#include "MantidAPI/HistogramValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"

#include <limits>
#include <sstream>

using Mantid::HistogramData::BinEdges;

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FlatCell)

using namespace Kernel;
using namespace API;
using DataObjects::EventWorkspace;
using DataObjects::EventWorkspace_const_sptr;
using DataObjects::EventWorkspace_sptr;

void FlatCell::init() {

  declareProperty(std::make_unique<WorkspaceProperty<API::Workspace>>("InputWorkspace", "", Direction::Input),
                  "The name of the input Workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<API::Workspace>>("OutputWorkspace", "", Direction::Output),
                  "The name of the Workspace containing the flat cell bins.");
}

/** Execution code.
 *  @throw std::invalid_argument If XMax is less than XMin
 */
void FlatCell::exec() {
  // Check for valid X limits
  int m_startX = 0;
  int m_endX = 0;
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  auto LAB = inputWS->readY(0)[2:16385];

  std::stringstream msg;
  msg << "LAB (" << LAB << ")";
  g_log.warning(msg.str());
  // throw std::invalid_argument(msg.str());
  // m_endX = getProperty("XMax");

  // if (m_startX > m_endX) {
  //   std::stringstream msg;
  //   msg << "XMax (" << m_endX << ") must be greater than XMin (" << m_startX << ")";
  //   g_log.error(msg.str());
  //   throw std::invalid_argument(msg.str());
  // }

  // // Copy indices from legacy property
  // std::vector<int64_t> spectraList = this->getProperty("SpectraList");
  // if (!spectraList.empty()) {
  //   if (!isDefault("InputWorkspaceIndexSet"))
  //     throw std::runtime_error("Cannot provide both InputWorkspaceIndexSet and "
  //                              "SpectraList at the same time.");
  //   setProperty("InputWorkspaceIndexSet", spectraList);
  //   g_log.warning("The 'SpectraList' property is deprecated. Use "
  //                 "'InputWorkspaceIndexSet' instead.");
  // }

  // MatrixWorkspace_sptr inputWS;
  // std::tie(inputWS, indexSet) = getWorkspaceAndIndices<MatrixWorkspace>("InputWorkspace");

  // // Only create the output workspace if it's different to the input one
  // MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  // if (outputWS != inputWS) {
  //   outputWS = inputWS->clone();
  //   setProperty("OutputWorkspace", outputWS);
  // }

  // if (std::dynamic_pointer_cast<const EventWorkspace>(inputWS)) {
  //   this->execEvent();
  // } else {
  //   MantidVec::difference_type startBin(0), endBin(0);

  //   // If the binning is the same throughout, we only need to find the index
  //   // limits once
  //   const bool commonBins = inputWS->isCommonBins();
  //   if (commonBins) {
  //     auto X = inputWS->binEdges(0);
  //   }

  // Progress progress(this, 0.0, 1.0, indexSet.size());
  // // Parallel running has problems with a race condition, leading to
  // // occaisional test failures and crashes

  // for (const auto wi : indexSet) {
  //   MantidVec::difference_type startBinLoop(startBin), endBinLoop(endBin);

  //   // Loop over masking each bin in the range
  //   for (auto j = static_cast<int>(startBinLoop); j < static_cast<int>(endBinLoop); ++j) {
  //     outputWS->maskBin(wi, j);
  //   }
  //   progress.report();
  // }
}
}

/** Execution code for EventWorkspaces
 */
void FlatCell::execEvent() {
  // To Do
}

} // namespace Mantid::Algorithms
