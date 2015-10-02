#include "MantidMDAlgorithms/CompactMD.h"
#include "MantidAPI/IMDIterator.h"
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace {
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
}

namespace Mantid {
namespace MDAlgorithms {

DECLARE_ALGORITHM(CompactMD)

const std::string CompactMD::name() const { return "CompactMD"; }

void CompactMD::init() {
  // input workspace to compact
  declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("InputWorkspace", "",
                                                           Direction::Input),
                  "MDHistoWorkspace to compact");
  // output workspace that will have been compacted
  declareProperty(new WorkspaceProperty<IMDWorkspace>("OutputWorkspace", "",
                                                      Direction::Output),
                  "Output compacted workspace");
}
void CompactMD::exec() {
  const IMDHistoWorkspace_sptr input_ws = this->getProperty("InputWorkspace");
  IMDWorkspace_sptr out_ws;

  const size_t nDimensions = input_ws->getNumDims();
  std::vector<Mantid::coord_t> minVector;
  std::vector<Mantid::coord_t> maxVector;

  // fill the min/max vectors with values per dimension.
  for (auto index = 0; index < nDimensions; index++) {
    minVector.push_back(input_ws->getDimension(index)->getMaximum());
    maxVector.push_back(input_ws->getDimension(index)->getMinimum());
  }
  // start our search for the first non-zero signal index.
  auto ws_iter = input_ws->createIterator();
  while (ws_iter->next()) {
    if (ws_iter->getSignal() == 0) {
      continue;
    } else {
      auto current_index = ws_iter->getLinearIndex();
      auto current_center = input_ws->getCenter(current_index);
      for (auto index = 0; index < input_ws->getNumDims(); index++) {
        if (current_center[index] > maxVector[index]) {
          maxVector[index] = current_center[index];
        }
        if (current_center[index] < minVector[index]) {
          minVector[index] = current_center[index];
        }
      }
    }
  }
  std::cout << "minX : " << minVector[0] << " maxX : " << maxVector[0] << "\n";
  std::cout << "minY : " << minVector[1] << " maxY : " << maxVector[1] << "\n";
  std::cout << "minZ : " << minVector[2] << " maxZ : " << maxVector[2] << "\n";

  auto pBinStrings = createPBinStringVector(minVector, maxVector, input_ws);
  
  auto cut_alg = this->createChildAlgorithm("CutMD");
  cut_alg->setProperty("InputWorkspace", input_ws);
  cut_alg->setProperty("OutputWorkspace", "temp");
  for (size_t iter = 0; iter < input_ws->getNumDims(); iter++){
      std::string propertyString = "P" + boost::lexical_cast<std::string>(iter+1) + "Bin";
      cut_alg->setProperty(propertyString, pBinStrings[iter]);
  }
  cut_alg->execute();

  IMDWorkspace_sptr temp = cut_alg->getProperty("OutputWorkspace");
  out_ws = temp;

  this->setProperty("OutputWorkspace", out_ws);
}

IMDHistoWorkspace_sptr
CompactMD::compactWorkspace(IMDHistoWorkspace_sptr workspace) {
  return workspace;
}
}
}