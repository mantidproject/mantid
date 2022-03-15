#include "MantidMDAlgorithms/IntegrateEllipsoids.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/Property.h"
#include "MantidMDAlgorithms/IntegrateEllipsoidsV1.h"
#include "MantidMDAlgorithms/IntegrateEllipsoidsV2.h"

#include <boost/algorithm/string.hpp>

using namespace Mantid::API;
using namespace Mantid::HistogramData;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

namespace Mantid::MDAlgorithms {

DECLARE_ALGORITHM(IntegrateEllipsoids)
//---------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string IntegrateEllipsoids::name() const { return "IntegrateEllipsoids"; }

/// Algorithm's version for identification. @see Algorithm::version
int IntegrateEllipsoids::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string IntegrateEllipsoids::category() const { return "Crystal\\Integration"; }
//---------------------------------------------------------------------

void IntegrateEllipsoids::init() {
  auto ws_valid = std::make_shared<CompositeValidator>();
  ws_valid->add<WorkspaceUnitValidator>("TOF");
  ws_valid->add<InstrumentValidator>();
  // the validator which checks if the workspace has axis

  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input, ws_valid),
      "An input MatrixWorkspace with time-of-flight units along "
      "X-axis and defined instrument with defined sample");

  declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>("PeaksWorkspace", "", Direction::InOut),
                  "Workspace with Peaks to be integrated. NOTE: The peaks MUST "
                  "be indexed with integer HKL values.");

  std::shared_ptr<BoundedValidator<double>> mustBePositive(new BoundedValidator<double>());
  mustBePositive->setLower(0.0);

  declareProperty("RegionRadius", .35, mustBePositive,
                  "Only events at most this distance from a peak will be "
                  "considered when integrating");

  declareProperty("SpecifySize", false, "If true, use the following for the major axis sizes, else use 3-sigma");

  declareProperty("PeakSize", .18, mustBePositive, "Half-length of major axis for peak ellipsoid");

  declareProperty("BackgroundInnerSize", .18, mustBePositive,
                  "Half-length of major axis for inner ellipsoidal surface of "
                  "background region");

  declareProperty("BackgroundOuterSize", .23, mustBePositive,
                  "Half-length of major axis for outer ellipsoidal surface of "
                  "background region");

  declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "The output PeaksWorkspace will be a copy of the input PeaksWorkspace "
                  "with the peaks' integrated intensities.");

  declareProperty("CutoffIsigI", EMPTY_DBL(), mustBePositive,
                  "Cuttoff for I/sig(i) when finding mean of half-length of "
                  "major radius in first pass when SpecifySize is false."
                  "Default is no second pass.");

  declareProperty("NumSigmas", 3,
                  "Number of sigmas to add to mean of half-length of "
                  "major radius for second pass when SpecifySize is false.");
  declareProperty("IntegrateInHKL", false, "If true, integrate in HKL space not Q space.");
  declareProperty("IntegrateIfOnEdge", true,
                  "Set to false to not integrate if peak radius is off edge of detector."
                  "Background will be scaled if background radius is off edge.");

  declareProperty("AdaptiveQBackground", false,
                  "Default is false.   If true, "
                  "BackgroundOuterRadius + AdaptiveQMultiplier * **|Q|** and "
                  "BackgroundInnerRadius + AdaptiveQMultiplier * **|Q|**");

  declareProperty("AdaptiveQMultiplier", 0.0,
                  "PeakRadius + AdaptiveQMultiplier * **|Q|** "
                  "so each peak has a "
                  "different integration radius.  Q includes the 2*pi factor.");

  declareProperty("UseOnePercentBackgroundCorrection", true,
                  "If this options is enabled, then the the top 1% of the "
                  "background will be removed"
                  "before the background subtraction.");

  // satellite realted properties
  declareProperty("SatelliteRegionRadius", EMPTY_DBL(), mustBePositive,
                  "Only events at most this distance from a satellite peak will be considered when integration");
  declareProperty("SatellitePeakSize", EMPTY_DBL(), mustBePositive,
                  "Half-length of major axis for satellite peak ellipsoid");
  declareProperty("ShareBackground", false, "Whether to use the same peak background region for satellite peaks.");
  declareProperty(
      "SatelliteBackgroundInnerSize", EMPTY_DBL(), mustBePositive,
      "Half-length of major axis for the inner ellipsoidal surface of background region of the satellite peak");
  declareProperty(
      "SatelliteBackgroundOuterSize", EMPTY_DBL(), mustBePositive,
      "Half-length of major axis for the outer ellipsoidal surface of background region of the satellite peak");
  declareProperty("GetUBFromPeaksWorkspace", false, "If true, UB is taken from peak workspace.");
}

int getIndexCount(PeaksWorkspace_sptr peakWorkspace) {
  int indexCount = 0;
  const int numPeaks = peakWorkspace->getNumberPeaks();
  for (int i = 0; i < numPeaks; ++i) {
    const auto peak = peakWorkspace->getPeak(i);
    if (peak.getHKL().norm2() > 0)
      indexCount += 1;
  }
  return indexCount;
}

void IntegrateEllipsoids::exec() {
  const bool isIntegrateInHKL = getProperty("IntegrateInHKL");
  const bool isGetUBFromPeaksWorkspace = getProperty("GetUBFromPeaksWorkspace");
  const bool shareBackground = getProperty("ShareBackground");

  const PeaksWorkspace_sptr peakWorkspace = getProperty("PeaksWorkspace");

  const int indexCount = getIndexCount(peakWorkspace);

  IAlgorithm_sptr alg;

  // detect which algo to run
  if ((isIntegrateInHKL || isGetUBFromPeaksWorkspace) && indexCount > 0 && !shareBackground) {
    // v1
    alg = createChildAlgorithm("IntegrateEllipsoidsV1", 0.0, 1.0);
  } else {
    // v2
    alg = createChildAlgorithm("IntegrateEllipsoidsV2", 0.0, 1.0);
  }
  alg->initialize();

  // forward properties to algo
  const std::vector<Property *> &props = alg->getProperties();
  for (auto prop : props) {
    if (prop) {
      if (boost::starts_with(prop->type(), "MatrixWorkspace")) {
        MatrixWorkspace_sptr workspace = getProperty(prop->name());
        alg->setProperty(prop->name(), workspace);
      } else if (boost::starts_with(prop->type(), "PeaksWorkspace")) {
        PeaksWorkspace_sptr workspace = getProperty(prop->name());
        alg->setProperty(prop->name(), workspace);
      } else {
        alg->setPropertyValue(prop->name(), getPropertyValue(prop->name()));
      }
    }
  }
  // childAlg->copyPropertiesFrom(*this); TODO look into this
  // run correct algo and return results
  alg->executeAsChildAlg();
  PeaksWorkspace_sptr outputWorkspace = alg->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace", outputWorkspace);
}
} // namespace Mantid::MDAlgorithms
