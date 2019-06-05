// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/CountReflections.h"
#include "MantidCrystal/PeakStatisticsTools.h"

#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceProperty.h"

#include "MantidDataObjects/PeaksWorkspace.h"

#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Crystal/PointGroupFactory.h"
#include "MantidGeometry/Crystal/ReflectionCondition.h"

#include "MantidKernel/ListValidator.h"

namespace Mantid {
namespace Crystal {

using Mantid::Kernel::Direction;

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CountReflections)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string CountReflections::name() const { return "CountReflections"; }

/// Algorithm's version for identification. @see Algorithm::version
int CountReflections::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CountReflections::category() const {
  return "Crystal\\Peaks";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CountReflections::summary() const {
  return "Calculates statistics for a PeaksWorkspace based on symmetry and "
         "counting reflections.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CountReflections::init() {
  declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "A workspace with peaks to calculate statistics for. Sample "
                  "with valid UB-matrix is required.");

  auto centeringSymbols = getAllReflectionConditionSymbols();
  declareProperty("LatticeCentering", centeringSymbols[0],
                  boost::make_shared<StringListValidator>(centeringSymbols),
                  "Lattice centering of the cell.");

  auto pointGroups = PointGroupFactory::Instance().getAllPointGroupSymbols();
  declareProperty(
      "PointGroup", "1", boost::make_shared<StringListValidator>(pointGroups),
      "Point group symmetry for completeness and redundancy calculations.");

  declareProperty(std::make_unique<PropertyWithValue<double>>(
                      "MinDSpacing", 1.0, Direction::Input),
                  "Minimum d-spacing for completeness calculation.");

  declareProperty(std::make_unique<PropertyWithValue<double>>(
                      "MaxDSpacing", 100.0, Direction::Input),
                  "Maximum d-spacing for completeness calculation.");

  declareProperty(std::make_unique<PropertyWithValue<int>>(
                      "UniqueReflections", 0, Direction::Output),
                  "Number of unique reflections in data set.");

  declareProperty(
      std::make_unique<PropertyWithValue<double>>("Completeness", 0.0,
                                                  Direction::Output),
      "Completeness of the data set as a fraction between 0 and 1.");

  declareProperty(std::make_unique<PropertyWithValue<double>>(
                      "Redundancy", 0.0, Direction::Output),
                  "Average redundancy in data set, depending on point group.");

  declareProperty(std::make_unique<PropertyWithValue<double>>(
                      "MultiplyObserved", 0.0, Direction::Output),
                  "Fraction of reflections with more than one observation.");

  declareProperty(
      std::make_unique<WorkspaceProperty<PeaksWorkspace>>(
          "MissingReflectionsWorkspace", "", Direction::Output,
          PropertyMode::Optional),
      "Reflections in specified d-range that are missing in input workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CountReflections::exec() {
  double dMin = getProperty("MinDSpacing");
  double dMax = getProperty("MaxDSpacing");

  PointGroup_sptr pointGroup =
      PointGroupFactory::Instance().createPointGroup(getProperty("PointGroup"));

  ReflectionCondition_sptr centering =
      getReflectionConditionBySymbol(getProperty("LatticeCentering"));

  PeaksWorkspace_sptr inputPeaksWorkspace = getProperty("InputWorkspace");

  UnitCell cell = inputPeaksWorkspace->sample().getOrientedLattice();

  PeakStatisticsTools::UniqueReflectionCollection reflections(
      cell, std::make_pair(dMin, dMax), pointGroup, centering);

  auto peaks = inputPeaksWorkspace->getPeaks();
  reflections.addObservations(peaks);

  double possibleUniqueReflections =
      static_cast<double>(reflections.getUniqueReflectionCount());

  size_t observedUniqueReflections =
      reflections.getObservedUniqueReflectionCount();

  double observedUniqueReflectionsD =
      static_cast<double>(observedUniqueReflections);

  size_t totalReflections = reflections.getObservedReflectionCount();

  if (peaks.size() > totalReflections) {
    g_log.information() << "There are " << (peaks.size() - totalReflections)
                        << " peaks in the input workspace that fall outside "
                           "the resolution limit and are not considered for "
                           "the calculations."
                        << std::endl;
  }

  double multiplyObservedReflections =
      static_cast<double>(reflections.getObservedUniqueReflectionCount(1));

  setProperty("UniqueReflections", static_cast<int>(observedUniqueReflections));
  setProperty("Completeness",
              observedUniqueReflectionsD / possibleUniqueReflections);
  setProperty("Redundancy", static_cast<double>(totalReflections) /
                                observedUniqueReflectionsD);
  setProperty("MultiplyObserved",
              multiplyObservedReflections / observedUniqueReflectionsD);

  PeaksWorkspace_sptr outputWorkspace =
      getPeaksWorkspace(inputPeaksWorkspace, reflections, pointGroup);

  if (outputWorkspace) {
    setProperty("MissingReflectionsWorkspace", outputWorkspace);
  }
}

/**
 * @brief CountReflections::getPeaksWorkspace
 *
 * This method expands the missing unique reflections to all reflections,
 * so that for example (001) would yield (001) and (00-1) for point group -1.
 *
 * Then these reflections are translated into peaks and put into the output-
 * workspace. This method could at some point probably move closer to (or into)
 * UniqueReflectionCollection.
 *
 * @param templateWorkspace :: Input workspace to clone if necessary.
 * @param reflections :: Vector of unique reflections.
 * @param pointGroup :: Point group to expand unique reflections.
 * @return :: PeaksWorkspace with missing reflections.
 */
PeaksWorkspace_sptr CountReflections::getPeaksWorkspace(
    const PeaksWorkspace_sptr &templateWorkspace,
    const PeakStatisticsTools::UniqueReflectionCollection &reflections,
    const PointGroup_sptr &pointGroup) const {
  std::string outputWorkspaceName =
      getPropertyValue("MissingReflectionsWorkspace");

  if (outputWorkspaceName.empty()) {
    return PeaksWorkspace_sptr();
  }

  PeaksWorkspace_sptr rawOutputPeaksWorkspace =
      getProperty("MissingReflectionsWorkspace");

  PeaksWorkspace_sptr outputPeaksWorkspace =
      boost::dynamic_pointer_cast<PeaksWorkspace>(rawOutputPeaksWorkspace);

  if (outputPeaksWorkspace != templateWorkspace) {
    outputPeaksWorkspace = templateWorkspace->clone();
  }

  const auto &missingPeaks = reflections.getUnobservedUniqueReflections();

  std::vector<Peak> peaks;
  peaks.reserve(missingPeaks.size() * pointGroup->order());

  for (const auto &reflection : missingPeaks) {
    auto hkls = pointGroup->getEquivalents(reflection);

    for (const auto &hkl : hkls) {
      Peak peak;
      peak.setHKL(hkl);

      peaks.emplace_back(peak);
    }
  }

  outputPeaksWorkspace->getPeaks().swap(peaks);

  return outputPeaksWorkspace;
}

} // namespace Crystal
} // namespace Mantid
