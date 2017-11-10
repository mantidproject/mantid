#include "MantidCrystal/StatisticsOfPeaksWorkspace.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Crystal/ReflectionCondition.h"
#include "MantidKernel/Utils.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ListValidator.h"

#include <fstream>

using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::PhysicalConstants;

namespace Mantid {
namespace Crystal {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(StatisticsOfPeaksWorkspace)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
StatisticsOfPeaksWorkspace::StatisticsOfPeaksWorkspace() {
  m_pointGroups = getAllPointGroups();
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void StatisticsOfPeaksWorkspace::init() {
  declareProperty(make_unique<WorkspaceProperty<PeaksWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "An input PeaksWorkspace with an instrument.");
  std::vector<std::string> propOptions;
  propOptions.reserve(m_pointGroups.size());
  for (auto &pointGroup : m_pointGroups)
    propOptions.push_back(pointGroup->getName());
  declareProperty("PointGroup", propOptions[0],
                  boost::make_shared<StringListValidator>(propOptions),
                  "Which point group applies to this crystal?");

  std::vector<std::string> centeringOptions;
  std::vector<ReflectionCondition_sptr> reflectionConditions =
      getAllReflectionConditions();
  centeringOptions.reserve(reflectionConditions.size());
  for (auto &reflectionCondition : reflectionConditions)
    centeringOptions.push_back(reflectionCondition->getName());
  declareProperty("LatticeCentering", centeringOptions[0],
                  boost::make_shared<StringListValidator>(centeringOptions),
                  "Appropriate lattice centering for the peaks.");

  declareProperty(make_unique<WorkspaceProperty<PeaksWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Output PeaksWorkspace");
  declareProperty(make_unique<WorkspaceProperty<ITableWorkspace>>(
                      "StatisticsTable", "StatisticsTable", Direction::Output),
                  "An output table workspace for the statistics of the peaks.");
  std::vector<std::string> sortTypes{"ResolutionShell", "Bank", "RunNumber",
                                     "Overall"};
  declareProperty("SortBy", sortTypes[0],
                  boost::make_shared<StringListValidator>(sortTypes),
                  "Sort the peaks by resolution shell in d-Spacing(default), "
                  "bank, run number, or only overall statistics.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void StatisticsOfPeaksWorkspace::exec() {

  ws = getProperty("InputWorkspace");
  std::string sortType = getProperty("SortBy");
  IPeaksWorkspace_sptr tempWS = WorkspaceFactory::Instance().createPeaks();
  // Copy over ExperimentInfo from input workspace
  tempWS->copyExperimentInfoFrom(ws.get());

  // We must sort the peaks
  std::vector<std::pair<std::string, bool>> criteria;
  if (sortType.compare(0, 2, "Re") == 0)
    criteria.push_back(std::pair<std::string, bool>("DSpacing", false));
  else if (sortType.compare(0, 2, "Ru") == 0)
    criteria.push_back(std::pair<std::string, bool>("RunNumber", true));
  criteria.push_back(std::pair<std::string, bool>("BankName", true));
  criteria.push_back(std::pair<std::string, bool>("h", true));
  criteria.push_back(std::pair<std::string, bool>("k", true));
  criteria.push_back(std::pair<std::string, bool>("l", true));
  ws->sort(criteria);

  // =========================================
  std::vector<Peak> peaks = ws->getPeaks();
  doSortHKL(ws, "Overall");
  if (sortType.compare(0, 2, "Ov") == 0)
    return;
  // run number
  std::string oldSequence;
  if (sortType.compare(0, 2, "Re") == 0) {
    double dspacing = peaks[0].getDSpacing();
    if (dspacing > 3.0)
      oldSequence = "Inf - 3.0";
    else if (dspacing > 2.5)
      oldSequence = "3.0 - 2.5";
    else if (dspacing > 2.0)
      oldSequence = "2.5 - 2.0";
    else if (dspacing > 1.5)
      oldSequence = "2.0 - 1.5";
    else if (dspacing > 1.0)
      oldSequence = "1.5 - 1.0";
    else if (dspacing > 0.5)
      oldSequence = "1.0 - 0.5";
    else
      oldSequence = "0.5 - 0.0";
  } else if (sortType.compare(0, 2, "Ru") == 0)
    oldSequence = std::to_string(peaks[0].getRunNumber());
  else
    oldSequence = peaks[0].getBankName();
  // Go through each peak at this run / bank
  for (int wi = 0; wi < ws->getNumberPeaks(); wi++) {

    Peak &p = peaks[wi];
    std::string sequence;
    if (sortType.compare(0, 2, "Re") == 0) {
      double dspacing = p.getDSpacing();
      if (dspacing > 3.0)
        sequence = "Inf - 3.0";
      else if (dspacing > 2.5)
        sequence = "3.0 - 2.5";
      else if (dspacing > 2.0)
        sequence = "2.5 - 2.0";
      else if (dspacing > 1.5)
        sequence = "2.0 - 1.5";
      else if (dspacing > 1.0)
        sequence = "1.5 - 1.0";
      else if (dspacing > 0.5)
        sequence = "1.0 - 0.5";
      else
        sequence = "0.5 - 0.0";
    } else if (sortType.compare(0, 2, "Ru") == 0)
      sequence = std::to_string(p.getRunNumber());
    else
      sequence = p.getBankName();

    if (sequence != oldSequence && tempWS->getNumberPeaks() > 0) {
      if (tempWS->getNumberPeaks() > 1)
        doSortHKL(tempWS, oldSequence);
      tempWS = WorkspaceFactory::Instance().createPeaks();
      // Copy over ExperimentInfo from input workspace
      tempWS->copyExperimentInfoFrom(ws.get());
      oldSequence = sequence;
    }
    tempWS->addPeak(p);
  }
  doSortHKL(tempWS, oldSequence);
}
//----------------------------------------------------------------------------------------------
/** Perform SortHKL on the output workspaces
 *
 * @param ws :: any PeaksWorkspace
 * @param runName :: string to put in statistics table
 */
void StatisticsOfPeaksWorkspace::doSortHKL(Mantid::API::Workspace_sptr ws,
                                           std::string runName) {
  std::string pointGroup = getPropertyValue("PointGroup");
  std::string latticeCentering = getPropertyValue("LatticeCentering");
  std::string wkspName = getPropertyValue("OutputWorkspace");
  std::string tableName = getPropertyValue("StatisticsTable");
  API::IAlgorithm_sptr statsAlg = createChildAlgorithm("SortHKL");
  statsAlg->setProperty("InputWorkspace", ws);
  statsAlg->setPropertyValue("OutputWorkspace", wkspName);
  statsAlg->setPropertyValue("StatisticsTable", tableName);
  statsAlg->setProperty("PointGroup", pointGroup);
  statsAlg->setProperty("LatticeCentering", latticeCentering);
  statsAlg->setProperty("RowName", runName);
  if (runName != "Overall")
    statsAlg->setProperty("Append", true);
  statsAlg->executeAsChildAlg();
  PeaksWorkspace_sptr statsWksp = statsAlg->getProperty("OutputWorkspace");
  ITableWorkspace_sptr tablews = statsAlg->getProperty("StatisticsTable");
  if (runName == "Overall")
    setProperty("OutputWorkspace", statsWksp);
  setProperty("StatisticsTable", tablews);
}

} // namespace Mantid
} // namespace Crystal
