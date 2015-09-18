#include "MantidMDAlgorithms/CompactMD.h"
#include "MantidAPI/IMDIterator.h"
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace Mantid {
namespace MDAlgorithms {
DECLARE_ALGORITHM(CompactMD);
const std::string CompactMD::name() const { return "CompactMD"; }


void CompactMD::init() {
    // input workspace to compact
  declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("InputWorkspace", "",
                                                           Direction::Input),
                  "MDHistoWorkspace to compact");
    // output workspace that will have been compacted
  declareProperty(new WorkspaceProperty<IMDWorkspace>(
                      "OutputWorkspace", "", Direction::Output),
                  "Output compacted workspace");
}
void CompactMD::exec() {
  const IMDHistoWorkspace_sptr input_ws = this->getProperty("InputWorkspace");
  IMDWorkspace_sptr out_ws;
  auto ws_iter = input_ws->createIterator();
  auto xbinWidth = input_ws->getXDimension()->getBinWidth();
  auto ybinWidth = input_ws->getYDimension()->getBinWidth();
  auto zbinWidth = input_ws->getZDimension()->getBinWidth();
  size_t first_non_zero_index = 0;
  size_t signals_read = 0;
  const double orig_minX = input_ws->getXDimension()->getMinimum();
  const double orig_minY = input_ws->getYDimension()->getMinimum();
  const double orig_minZ = input_ws->getZDimension()->getMinimum();

  double minX = input_ws->getXDimension()->getMinimum();
  double minY = input_ws->getYDimension()->getMinimum();
  double minZ = input_ws->getZDimension()->getMinimum();

  const double orig_maxX = input_ws->getXDimension()->getMaximum(); 
  const double orig_maxY = input_ws->getYDimension()->getMaximum();
  const double orig_maxZ = input_ws->getZDimension()->getMaximum();

  double maxX = input_ws->getXDimension()->getMaximum(); 
  double maxY = input_ws->getYDimension()->getMaximum();
  double maxZ = input_ws->getZDimension()->getMaximum();

  size_t nBinsZ = input_ws->getZDimension()->getNBins();
  size_t nBinsY = input_ws->getYDimension()->getNBins();
  size_t nBinsX = input_ws->getXDimension()->getNBins();

  for (size_t z = 0; z < nBinsZ; z++)
  {
      for (size_t y = 0; y < nBinsY; y++)
      {
          for (size_t x = 0; x < nBinsX; x++)
          {
              if (input_ws->getSignalAt(x,y,z) == 0 ){
                  break;
              }
              else{
                  
                  if (input_ws->getXDimension()->getX(x) < minX) {minX = orig_minX + xbinWidth*x;}
                  if (input_ws->getXDimension()->getX(x) > maxX) {maxX = orig_maxX + xbinWidth*x;}
                  if (input_ws->getYDimension()->getX(y) < minY) {minY = orig_minY + ybinWidth*y;}
                  if (input_ws->getYDimension()->getX(y) > maxY) {maxY = orig_maxY + ybinWidth*y;}
                  if (input_ws->getZDimension()->getX(z) < minZ) {minZ = orig_minZ + zbinWidth*z;}
                  if (input_ws->getZDimension()->getX(z) > maxZ) {maxZ = orig_maxZ + zbinWidth*z;}
              }
          }
      }
  }
  std::cout << "minX : " << minX << " maxX : " << maxX << "\n";
  std::cout << "minY : " << minY << " maxY : " << maxY << "\n";
  std::cout << "minZ : " << minZ << " maxZ : " << maxZ << "\n";
  auto cut_alg = this->createChildAlgorithm("CutMD");
  cut_alg->setProperty("InputWorkspace", input_ws);
  cut_alg->setProperty("OutputWorkspace","temp");
  cut_alg->setProperty("P1Bin",boost::lexical_cast<std::string>(minX)+",0,"+ boost::lexical_cast<std::string>(maxX));
  cut_alg->setProperty("P2Bin",boost::lexical_cast<std::string>(minY)+",0,"+boost::lexical_cast<std::string>(maxY));
  cut_alg->setProperty("P3Bin",boost::lexical_cast<std::string>(minZ)+",0,"+boost::lexical_cast<std::string>(maxZ));
  cut_alg->execute();

  std::cout << "First non-zero: " << first_non_zero_index << "\n";
  std::cout << "Signals read: " << signals_read << "\n";

  IMDWorkspace_sptr temp = cut_alg->getProperty("OutputWorkspace");
  out_ws = temp;

  this->setProperty("OutputWorkspace", out_ws);
}

IMDHistoWorkspace_sptr CompactMD::compactWorkspace(IMDHistoWorkspace_sptr workspace,
    signal_t threshold){return workspace;}

}
}