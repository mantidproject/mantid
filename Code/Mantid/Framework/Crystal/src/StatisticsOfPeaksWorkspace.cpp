#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidCrystal/StatisticsOfPeaksWorkspace.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Utils.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ListValidator.h"

#include <boost/math/special_functions/fpclassify.hpp>
#include <fstream>
#include <Poco/File.h>
#include <Poco/Path.h>
#include "boost/assign.hpp"

using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::PhysicalConstants;
using namespace boost::assign;

namespace Mantid {
namespace Crystal {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(StatisticsOfPeaksWorkspace)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
StatisticsOfPeaksWorkspace::StatisticsOfPeaksWorkspace() { m_pointGroups = getAllPointGroups(); }

//----------------------------------------------------------------------------------------------
/** Destructor
 */
StatisticsOfPeaksWorkspace::~StatisticsOfPeaksWorkspace() {}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void StatisticsOfPeaksWorkspace::init() {
  declareProperty(new WorkspaceProperty<PeaksWorkspace>("InputWorkspace", "",
                                                        Direction::Input),
                  "An input PeaksWorkspace with an instrument.");
  std::vector<std::string> propOptions;
  for (size_t i = 0; i < m_pointGroups.size(); ++i)
    propOptions.push_back(m_pointGroups[i]->getName());
  declareProperty("PointGroup", propOptions[0],
                  boost::make_shared<StringListValidator>(propOptions),
                  "Which point group applies to this crystal?");

  declareProperty(new WorkspaceProperty<PeaksWorkspace>("OutputWorkspace", "",
                                                        Direction::Output),
                  "Output PeaksWorkspace");
  declareProperty(new WorkspaceProperty<ITableWorkspace>(
                      "StatisticsTable", "StaticticsTable", Direction::Output),
                  "An output table workspace for the statistics of the peaks.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void StatisticsOfPeaksWorkspace::exec() {

  ws = getProperty("InputWorkspace");
  IPeaksWorkspace_sptr tempWS = WorkspaceFactory::Instance().createPeaks();
  // Copy over ExperimentInfo from input workspace
  tempWS->copyExperimentInfoFrom(ws.get());

  // We must sort the peaks
  std::vector<std::pair<std::string, bool>> criteria;
  criteria.push_back(std::pair<std::string, bool>("RunNumber", true));
  criteria.push_back(std::pair<std::string, bool>("BankName", true));
  criteria.push_back(std::pair<std::string, bool>("h", true));
  criteria.push_back(std::pair<std::string, bool>("k", true));
  criteria.push_back(std::pair<std::string, bool>("l", true));
  ws->sort(criteria);



  // =========================================
  std::vector<Peak> peaks = ws->getPeaks();
  doSortHKL(ws, "Overall");
  // run number
  int oldSequence = peaks[0].getRunNumber();
  // Go through each peak at this run / bank
  for (int wi = 0; wi < ws->getNumberPeaks(); wi++) {

    Peak &p = peaks[wi];
    int sequence = p.getRunNumber();

    if (sequence != oldSequence && tempWS->getNumberPeaks()>0) {
      doSortHKL(tempWS, boost::lexical_cast<std::string>(oldSequence));
      tempWS = WorkspaceFactory::Instance().createPeaks();
      // Copy over ExperimentInfo from input workspace
      tempWS->copyExperimentInfoFrom(ws.get());
      oldSequence = sequence;
    }
    tempWS->addPeak(p);
  }
  doSortHKL(tempWS, boost::lexical_cast<std::string>(oldSequence));
}
//----------------------------------------------------------------------------------------------
/** Perform SortHKL on the output workspaces
 *
 * @param ws :: any PeaksWorkspace
 */
void StatisticsOfPeaksWorkspace::doSortHKL(Mantid::API::Workspace_sptr ws, std::string runName){
  std::string pointGroup = getPropertyValue("PointGroup");
  std::string wkspName = getPropertyValue("OutputWorkspace");
  std::string tableName = getPropertyValue("StatisticsTable");
  API::IAlgorithm_sptr statsAlg = createChildAlgorithm("SortHKL");
  statsAlg->setProperty("InputWorkspace", ws);
  statsAlg->setPropertyValue("OutputWorkspace", wkspName);
  statsAlg->setPropertyValue("StatisticsTable", tableName);
  statsAlg->setProperty("PointGroup", pointGroup);
  statsAlg->setProperty("RowName", runName);
  if (runName.compare("Overall") != 0)
    statsAlg->setProperty("Append", true);
  statsAlg->executeAsChildAlg();
  PeaksWorkspace_sptr statsWksp =
      statsAlg->getProperty("OutputWorkspace");
  ITableWorkspace_sptr  tablews =
      statsAlg->getProperty("StatisticsTable");
  if (runName.compare("Overall") == 0)
    setProperty("OutputWorkspace", statsWksp);
  setProperty("StatisticsTable", tablews);
}

} // namespace Mantid
} // namespace Crystal
