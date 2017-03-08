#include "MantidCrystal/CountPeaks.h"
#include "MantidCrystal/PeakStatisticsTools.h"

#include "MantidAPI/Sample.h"

#include "MantidDataObjects/PeaksWorkspace.h"

#include "MantidGeometry/Crystal/PointGroupFactory.h"
#include "MantidGeometry/Crystal/ReflectionCondition.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

#include "MantidKernel/make_unique.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid {
namespace Crystal {

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;

using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CountPeaks)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string CountPeaks::name() const { return "CountPeaks"; }

/// Algorithm's version for identification. @see Algorithm::version
int CountPeaks::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CountPeaks::category() const {
  return "TODO: FILL IN A CATEGORY";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CountPeaks::summary() const {
  return "TODO: FILL IN A SUMMARY";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CountPeaks::init() {
  declareProperty(Kernel::make_unique<WorkspaceProperty<PeaksWorkspace>>(
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

  declareProperty(Kernel::make_unique<PropertyWithValue<double>>(
                      "MinDSpacing", 1.0, Direction::Input),
                  "Minimum d-spacing for completeness calculation.");

  declareProperty(Kernel::make_unique<PropertyWithValue<double>>(
                      "MaxDSpacing", 100.0, Direction::Input),
                  "Maximum d-spacing for completeness calculation.");

  declareProperty(Kernel::make_unique<PropertyWithValue<int>>(
                      "UniqueReflections", 0, Direction::Output),
                  "Number of unique reflections in data set.");

  declareProperty(
      Kernel::make_unique<PropertyWithValue<double>>("Completeness", 0.0,
                                                     Direction::Output),
      "Completeness of the data set as a fraction between 0 and 1.");

  declareProperty(Kernel::make_unique<PropertyWithValue<double>>(
                      "Redundancy", 0.0, Direction::Output),
                  "Average redundancy in data set, depending on point group.");

  declareProperty(Kernel::make_unique<PropertyWithValue<double>>(
                      "MultiplyObserved", 0.0, Direction::Output),
                  "Fraction of reflections with more than one observation.");

  //  declareProperty(
  //      Kernel::make_unique<WorkspaceProperty<PeaksWorkspace>>(
  //          "OutputWorkspace", "", Direction::Output),
  //      "Reflections in specified d-range that are missing in input
  //      workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CountPeaks::exec() {
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

  reflections.addObservations(inputPeaksWorkspace->getPeaks());

  double possibleUniqueReflections =
      static_cast<double>(reflections.getUniqueReflectionCount());

  double observedUniqueReflections =
      static_cast<double>(reflections.getObservedUniqueReflectionCount());

  double totalReflections =
      static_cast<double>(reflections.getObservedReflectionCount());

  double multiplyObservedReflections =
      static_cast<double>(reflections.getObservedUniqueReflectionCount(1));

  setProperty("UniqueReflections",
              static_cast<int>(round(observedUniqueReflections)));
  setProperty("Completeness",
              observedUniqueReflections / possibleUniqueReflections);
  setProperty("Redundancy", totalReflections / observedUniqueReflections);
  setProperty("MultiplyObserved",
              multiplyObservedReflections / observedUniqueReflections);
}

} // namespace Crystal
} // namespace Mantid
