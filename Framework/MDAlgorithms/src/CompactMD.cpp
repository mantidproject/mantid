#include "MantidMDAlgorithms/CompactMD.h"
#include "MantidAPI/IMDIterator.h"
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace {
// helper method to create a string from min and max extents (with non-zero
// signals)
// ready to be used as the PBins for IntegrateMDHistoWorkspace algorithm in
// exec.
std::vector<std::string>
createPBinStringVector(std::vector<Mantid::coord_t> minVector,
                       std::vector<Mantid::coord_t> maxVector,
                       IMDHistoWorkspace_sptr inputWs) {
  size_t numDims = inputWs->getNumDims();
  std::vector<std::string> pBinStrVector;
  for (size_t iter = 0; iter < numDims; iter++) {
    if (minVector[iter] >= maxVector[iter]) {
      std::cerr << "Minimum extent of non-zero signal must be LESS than the "
                   "maximum extent with non-zero signal"
                << std::endl;
      // break;
    }
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
}

namespace Mantid {
namespace MDAlgorithms {

DECLARE_ALGORITHM(CompactMD)

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
}

void CompactMD::init() {
  // input workspace to compact
  declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("InputWorkspace", "",
                                                           Direction::Input),
                  "MDHistoWorkspace to compact");
  // output workspace that will have been compacted
  declareProperty(new WorkspaceProperty<IMDHistoWorkspace>(
                      "OutputWorkspace", "", Direction::Output),
                  "Output compacted workspace");
}
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
    std::string propertyString =
        "P" + boost::lexical_cast<std::string>(iter + 1) + "Bin";
    cut_alg->setProperty(propertyString, pBinStrings[iter]);
  }
  cut_alg->execute();

  // retrieve the output workspace from IntegrateMDHistoWorkspace
  IMDHistoWorkspace_sptr temp = cut_alg->getProperty("OutputWorkspace");
  out_ws = temp;
  // set output workspace of CompactMD to output of IntegrateMDHistoWorkspace
  this->setProperty("OutputWorkspace", out_ws);
}
}
}