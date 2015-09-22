#include "MantidMDAlgorithms/CompactMD.h"
#include "MantidAPI/IMDIterator.h"
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

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
  /* auto xbinWidth = input_ws->getXDimension()->getBinWidth();
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

                   if (input_ws->getXDimension()->getX(x) < minX) {minX =
  orig_minX + xbinWidth*x;}
                   if (input_ws->getXDimension()->getX(x) > maxX) {maxX =
  orig_maxX + xbinWidth*x;}
                   if (input_ws->getYDimension()->getX(y) < minY) {minY =
  orig_minY + ybinWidth*y;}
                   if (input_ws->getYDimension()->getX(y) > maxY) {maxY =
  orig_maxY + ybinWidth*y;}
                   if (input_ws->getZDimension()->getX(z) < minZ) {minZ =
  orig_minZ + zbinWidth*z;}
                   if (input_ws->getZDimension()->getX(z) > maxZ) {maxZ =
  orig_maxZ + zbinWidth*z;}
               }
           }
       }
   }


  */
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
   while(ws_iter->next()){
      if (ws_iter->getSignal() == 0) {
        continue;
      } else {
        auto current_index =  ws_iter->getLinearIndex();
        auto current_center = input_ws->getCenter(current_index);
        for (auto index = 0; index < input_ws->getNumDims(); index++){
            if (current_center[index] > maxVector[index]){maxVector[index] = current_center[index];}
            if (current_center[index] < minVector[index]){minVector[index] = current_center[index];}
        }
        
      }
    }
  std::cout << "minX : " << minVector[0] << " maxX : " << maxVector[0] << "\n";
  std::cout << "minY : " << minVector[1] << " maxY : " << maxVector[1] << "\n";
  std::cout << "minZ : " << minVector[2] << " maxZ : " << maxVector[2] << "\n";
  auto cut_alg = this->createChildAlgorithm("CutMD");
  cut_alg->setProperty("InputWorkspace", input_ws);
  cut_alg->setProperty("OutputWorkspace", "temp");
  cut_alg->setProperty("P1Bin",
      boost::lexical_cast<std::string>(minVector[0]-(input_ws->getDimension(0)->getBinWidth()*0.5)) + ",0," +
                           boost::lexical_cast<std::string>(maxVector[0]+(input_ws->getDimension(0)->getBinWidth()*0.5)));
  cut_alg->setProperty("P2Bin",
                       boost::lexical_cast<std::string>(minVector[1]-(input_ws->getDimension(1)->getBinWidth()*0.5)) + ",0," +
                           boost::lexical_cast<std::string>(maxVector[1]+(input_ws->getDimension(1)->getBinWidth()*0.5)));
  cut_alg->setProperty("P3Bin",
                       boost::lexical_cast<std::string>(minVector[2]-(input_ws->getDimension(2)->getBinWidth()*0.5)) + ",0," +
                           boost::lexical_cast<std::string>(maxVector[2]+(input_ws->getDimension(2)->getBinWidth()*0.5)));
  cut_alg->setProperty("P4Bin",boost::lexical_cast<std::string>(minVector[3]-(input_ws->getDimension(3)->getBinWidth()*0.5)) + ",0," +
                           boost::lexical_cast<std::string>(maxVector[3]+(input_ws->getDimension(3)->getBinWidth()*0.5)));
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