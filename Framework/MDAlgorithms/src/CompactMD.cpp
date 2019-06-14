// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/CompactMD.h"
#include "MantidAPI/IMDIterator.h"

#include <boost/lexical_cast.hpp>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace {
/**
 * helper method to create a string from min and max extents (with non-zero
 * signals) ready to be used as the PBins for IntegrateMDHistoWorkspace
 * algorithm in
 * exec
 * @param minVector : Vector containing the minimum extents that we will crop
 * to.
 * @param maxVector : Vector containing the maximum extents that we will crop
 * to.
 * @param inputWs : Used in the calculation from centre to bin edges
 * @return : a string vector of binning parameters for IntegrateMDHistoWorkspace
 * to take as input.
 */
std::vector<std::string>
createPBinStringVector(std::vector<Mantid::coord_t> minVector,
                       std::vector<Mantid::coord_t> maxVector,
                       IMDHistoWorkspace_sptr inputWs) {
  size_t numDims = inputWs->getNumDims();
  std::vector<std::string> pBinStrVector;
  for (size_t iter = 0; iter < numDims; iter++) {
    // creating pbin string using Min and Max Centre positions
    auto pBinStr = boost::lexical_cast<std::string>(
                       minVector[iter] -
                       (inputWs->getDimension(iter)->getBinWidth() * 0.5)) +
                   ",0," +
                   boost::lexical_cast<std::string>(
                       maxVector[iter] +
                       (inputWs->getDimension(iter)->getBinWidth() * 0.5));
    pBinStrVector.push_back(pBinStr);
  }
  return pBinStrVector;
}
} // namespace

namespace Mantid {
namespace MDAlgorithms {

DECLARE_ALGORITHM(CompactMD)

/**
 * Finding the centre points of Bins with non-zero signal values
 * we then compare this centre to minimum and maximum centres we have
 * to get the minimum and maximum extents of the workspace that has non-zero
 * signal values in the Bins.
 * @param inputWs : The workspace that will be iterated over to find the
 * extents.
 * @param minVec : Vector used to stored the minimum extent in each dimension
 * @param maxVec : Vector used to stored the maximum extents in each dimension
 */

void CompactMD::findFirstNonZeroMinMaxExtents(
    IMDHistoWorkspace_sptr inputWs, std::vector<Mantid::coord_t> &minVec,
    std::vector<Mantid::coord_t> &maxVec) {
  auto ws_iter = inputWs->createIterator();
  do {
    if (ws_iter->getSignal() == 0) {
      // if signal is 0 then go to next index
      continue;
    } else {
      // we have found a non-zero signal we need to compare
      // the position of the bin with our Min and Max values
      auto current_index = ws_iter->getLinearIndex();
      auto current_center = inputWs->getCenter(current_index);
      for (size_t index = 0; index < inputWs->getNumDims(); index++) {
        if (current_center[index] > maxVec[index]) {
          // set new maximum
          maxVec[index] = current_center[index];
        }
        if (current_center[index] < minVec[index]) {
          // set new minimum
          minVec[index] = current_center[index];
        }
      }
    }
  } while (ws_iter->next());
  // check that min/max vector have changed, if not then we set them to original
  // extents of the workspace
  for (size_t index = 0; index < inputWs->getNumDims(); index++) {
    // if min/max for a dimension haven't changed then there were
    // no signals found in that dimension. We must set the min and max
    // for that dimension to the actual min and max extents respectively
    // which will stop that dimension being cropped or causing errors
    // when passed to IntegrateMDHistoWorkspace
    if (minVec[index] == inputWs->getDimension(index)->getMaximum()) {
      minVec[index] = inputWs->getDimension(index)->getMinimum();
    }
    if (maxVec[index] == inputWs->getDimension(index)->getMinimum()) {
      maxVec[index] = inputWs->getDimension(index)->getMaximum();
    }
  }
}

/**
 * Initiliase the algorithm's properties.
 */
void CompactMD::init() {
  // input workspace to compact
  declareProperty(std::make_unique<WorkspaceProperty<IMDHistoWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "MDHistoWorkspace to compact");
  // output workspace that will have been compacted
  declareProperty(std::make_unique<WorkspaceProperty<IMDHistoWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Output compacted workspace");
}
/**
 * Execute the algorithm.
 */
void CompactMD::exec() {
  const IMDHistoWorkspace_sptr input_ws = this->getProperty("InputWorkspace");
  IMDWorkspace_sptr out_ws;

  const size_t nDimensions = input_ws->getNumDims();
  std::vector<Mantid::coord_t> minVector;
  std::vector<Mantid::coord_t> maxVector;

  // fill the min/max vectors with values per dimension.
  for (size_t index = 0; index < nDimensions; index++) {
    minVector.push_back(input_ws->getDimension(index)->getMaximum());
    maxVector.push_back(input_ws->getDimension(index)->getMinimum());
  }
  // start our search for the first non-zero signal index.
  findFirstNonZeroMinMaxExtents(input_ws, minVector, maxVector);
  auto pBinStrings = createPBinStringVector(minVector, maxVector, input_ws);
  // creating IntegrateMDHistoWorkspace algorithm to crop our workspace.
  auto cut_alg = this->createChildAlgorithm("IntegrateMDHistoWorkspace");
  cut_alg->setProperty("InputWorkspace", input_ws);
  cut_alg->setProperty("OutputWorkspace", "temp");
  // setting property PxBin depending on the number of dimensions the
  // input workspace has.
  for (size_t iter = 0; iter < input_ws->getNumDims(); iter++) {
    std::string propertyString = "P" + std::to_string(iter + 1) + "Bin";
    cut_alg->setProperty(propertyString, pBinStrings[iter]);
  }
  cut_alg->execute();

  // retrieve the output workspace from IntegrateMDHistoWorkspace
  IMDHistoWorkspace_sptr temp = cut_alg->getProperty("OutputWorkspace");
  out_ws = temp;
  // set output workspace of CompactMD to output of IntegrateMDHistoWorkspace
  this->setProperty("OutputWorkspace", out_ws);
}
} // namespace MDAlgorithms
} // namespace Mantid
