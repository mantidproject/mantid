// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/StatisticsOfPeaksWorkspace.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Crystal/ReflectionCondition.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Utils.h"

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
  declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "An input PeaksWorkspace with an instrument.");
  std::vector<std::string> propOptions;
  propOptions.reserve(2 * m_pointGroups.size() + 5);
  std::transform(m_pointGroups.cbegin(), m_pointGroups.cend(),
                 std::back_inserter(propOptions),
                 [](const auto &group) { return group->getSymbol(); });
  std::transform(m_pointGroups.cbegin(), m_pointGroups.cend(),
                 std::back_inserter(propOptions),
                 [](const auto &group) { return group->getName(); });
  // Scripts may have Orthorhombic misspelled from past bug in PointGroupFactory
  propOptions.emplace_back("222 (Orthorombic)");
  propOptions.emplace_back("mm2 (Orthorombic)");
  propOptions.emplace_back("2mm (Orthorombic)");
  propOptions.emplace_back("m2m (Orthorombic)");
  propOptions.emplace_back("mmm (Orthorombic)");
  declareProperty("PointGroup", propOptions[0],
                  boost::make_shared<StringListValidator>(propOptions),
                  "Which point group applies to this crystal?");

  std::vector<std::string> centeringOptions;
  const std::vector<ReflectionCondition_sptr> reflectionConditions =
      getAllReflectionConditions();
  centeringOptions.reserve(2 * reflectionConditions.size());
  std::transform(reflectionConditions.cbegin(), reflectionConditions.cend(),
                 std::back_inserter(centeringOptions),
                 [](const auto &condition) { return condition->getSymbol(); });
  std::transform(reflectionConditions.cbegin(), reflectionConditions.cend(),
                 std::back_inserter(centeringOptions),
                 [](const auto &condition) { return condition->getName(); });
  declareProperty("LatticeCentering", centeringOptions[0],
                  boost::make_shared<StringListValidator>(centeringOptions),
                  "Appropriate lattice centering for the peaks.");

  declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Output PeaksWorkspace");
  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>(
                      "StatisticsTable", "StatisticsTable", Direction::Output),
                  "An output table workspace for the statistics of the peaks.");
  const std::vector<std::string> sortTypes{"ResolutionShell", "Bank",
                                           "RunNumber", "Overall"};
  declareProperty("SortBy", sortTypes[0],
                  boost::make_shared<StringListValidator>(sortTypes),
                  "Sort the peaks by resolution shell in d-Spacing(default), "
                  "bank, run number, or only overall statistics.");
  const std::vector<std::string> equivTypes{"Mean", "Median"};
  declareProperty("EquivalentIntensities", equivTypes[0],
                  boost::make_shared<StringListValidator>(equivTypes),
                  "Replace intensities by mean(default), "
                  "or median.");
  declareProperty(std::make_unique<PropertyWithValue<double>>(
                      "SigmaCritical", 3.0, Direction::Input),
                  "Removes peaks whose intensity deviates more than "
                  "SigmaCritical from the mean (or median).");
  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "EquivalentsWorkspace", "EquivalentIntensities", Direction::Output),
      "Output Equivalent Intensities");
  declareProperty("WeightedZScore", false,
                  "Use weighted ZScore if true.\n"
                  "If false, standard ZScore (default).");
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
  std::string equivalentIntensities = getPropertyValue("EquivalentIntensities");
  double sigmaCritical = getProperty("SigmaCritical");
  bool weightedZ = getProperty("WeightedZScore");
  API::IAlgorithm_sptr statsAlg = createChildAlgorithm("SortHKL");
  statsAlg->setProperty("InputWorkspace", ws);
  statsAlg->setPropertyValue("OutputWorkspace", wkspName);
  statsAlg->setPropertyValue("StatisticsTable", tableName);
  statsAlg->setProperty("PointGroup", pointGroup);
  statsAlg->setProperty("LatticeCentering", latticeCentering);
  statsAlg->setProperty("RowName", runName);
  if (runName != "Overall")
    statsAlg->setProperty("Append", true);
  statsAlg->setPropertyValue("EquivalentIntensities", equivalentIntensities);
  statsAlg->setProperty("SigmaCritical", sigmaCritical);
  statsAlg->setProperty("WeightedZScore", weightedZ);
  statsAlg->executeAsChildAlg();
  PeaksWorkspace_sptr statsWksp = statsAlg->getProperty("OutputWorkspace");
  ITableWorkspace_sptr tablews = statsAlg->getProperty("StatisticsTable");
  MatrixWorkspace_sptr equivws = statsAlg->getProperty("EquivalentsWorkspace");
  if (runName == "Overall")
    setProperty("OutputWorkspace", statsWksp);
  setProperty("StatisticsTable", tablews);
  setProperty("EquivalentsWorkspace", equivws);
}

} // namespace Crystal
} // namespace Mantid
