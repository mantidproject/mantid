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
                      "StatisticsTable", "StatisticsTable", Direction::Output),
                  "An output table workspace for the statistics of the peaks.");
  std::vector<std::string> sortTypes;
  sortTypes.push_back("ResolutionShell");
  sortTypes.push_back("Bank");
  sortTypes.push_back("RunNumber");
  sortTypes.push_back("Overall");
  declareProperty("SortBy", sortTypes[0],
                  boost::make_shared<StringListValidator>(sortTypes),
                  "Sort the peaks by bank, run number(default) or only overall statistics.");
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
    criteria.push_back(std::pair<std::string, bool>("wavelength", false));
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
  if (sortType.compare(0, 2, "Ov") == 0) return;
  // run number
  std::string oldSequence;
  if (sortType.compare(0, 2, "Re") == 0)
  {
    double wavelength = peaks[0].getWavelength();
    if (wavelength > 3.0) oldSequence = "first";
    else if (wavelength > 2.5) oldSequence = "second";
    else if (wavelength > 2.0) oldSequence = "third";
    else if (wavelength > 1.5) oldSequence = "fourth";
    else if (wavelength > 1.0) oldSequence = "fifth";
    else if (wavelength > 0.5) oldSequence = "sixth";
    else oldSequence = "seventh";
  }
  else if (sortType.compare(0, 2, "Ru") == 0)
    oldSequence = boost::lexical_cast<std::string>(peaks[0].getRunNumber());
  else oldSequence= peaks[0].getBankName();
  // Go through each peak at this run / bank
  for (int wi = 0; wi < ws->getNumberPeaks(); wi++) {

    Peak &p = peaks[wi];
    std::string sequence;
    if (sortType.compare(0, 2, "Re") == 0)
    {
          double wavelength = p.getWavelength();
          if (wavelength > 3.0) sequence = "first";
          else if (wavelength > 2.5) sequence = "second";
          else if (wavelength > 2.0) sequence = "third";
          else if (wavelength > 1.5) sequence = "fourth";
          else if (wavelength > 1.0) sequence = "fifth";
          else if (wavelength > 0.5) sequence = "sixth";
          else sequence = "seventh";
    }
    else if (sortType.compare(0, 2, "Ru") == 0)
      sequence = boost::lexical_cast<std::string>(p.getRunNumber());
    else sequence= p.getBankName();

    if (sequence.compare(oldSequence) !=0 && tempWS->getNumberPeaks()>0) {
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
 * @runName :: string to put in statistics table
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
