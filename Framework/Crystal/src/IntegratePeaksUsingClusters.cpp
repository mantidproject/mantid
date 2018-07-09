#include "MantidCrystal/IntegratePeaksUsingClusters.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidCrystal/ConnectedComponentLabeling.h"
#include "MantidCrystal/HardThresholdBackground.h"
#include "MantidCrystal/ICluster.h"
#include "MantidCrystal/PeakClusterProjection.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/Utils.h"

#include <cmath>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::Crystal::ConnectedComponentMappingTypes;

namespace Mantid {
namespace Crystal {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(IntegratePeaksUsingClusters)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string IntegratePeaksUsingClusters::name() const {
  return "IntegratePeaksUsingClusters";
}

/// Algorithm's version for identification. @see Algorithm::version
int IntegratePeaksUsingClusters::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string IntegratePeaksUsingClusters::category() const {
  return "MDAlgorithms\\Peaks;Crystal\\Integration";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void IntegratePeaksUsingClusters::init() {
  declareProperty(make_unique<WorkspaceProperty<IMDHistoWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "Input md workspace.");
  declareProperty(make_unique<WorkspaceProperty<PeaksWorkspace>>(
                      "PeaksWorkspace", "", Direction::Input),
                  "A PeaksWorkspace containing the peaks to integrate.");

  auto positiveValidator = boost::make_shared<BoundedValidator<double>>();
  positiveValidator->setExclusive(true);
  positiveValidator->setLower(0);

  auto compositeValidator = boost::make_shared<CompositeValidator>();
  compositeValidator->add(positiveValidator);
  compositeValidator->add(boost::make_shared<MandatoryValidator<double>>());

  declareProperty(make_unique<PropertyWithValue<double>>(
                      "Threshold", 0, compositeValidator, Direction::Input),
                  "Threshold signal above which to consider peaks");

  std::array<std::string, 3> normalizations = {
      {"NoNormalization", "VolumeNormalization", "NumEventsNormalization"}};

  declareProperty("Normalization", normalizations[1],
                  Kernel::IValidator_sptr(
                      new Kernel::ListValidator<std::string>(normalizations)),
                  "Normalization to use with Threshold. Defaults to "
                  "VolumeNormalization to account for different binning.");

  declareProperty(make_unique<WorkspaceProperty<PeaksWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output integrated peaks workspace.");
  declareProperty(make_unique<WorkspaceProperty<IMDHistoWorkspace>>(
                      "OutputWorkspaceMD", "", Direction::Output),
                  "MDHistoWorkspace containing the labeled clusters used by "
                  "the algorithm.");
}

/**
 * Get the normalization. For use with iterators + background strategies.
 * @return Chosen normalization
 */
MDNormalization IntegratePeaksUsingClusters::getNormalization() {
  std::string normProp = getPropertyValue("Normalization");
  Mantid::API::MDNormalization normalization;
  if (normProp == "NoNormalization") {
    normalization = NoNormalization;
  } else if (normProp == "VolumeNormalization") {
    normalization = VolumeNormalization;
  } else {
    normalization = NumEventsNormalization;
  }
  return normalization;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void IntegratePeaksUsingClusters::exec() {
  IMDHistoWorkspace_sptr mdWS = getProperty("InputWorkspace");
  PeaksWorkspace_sptr inPeakWS = getProperty("PeaksWorkspace");
  PeaksWorkspace_sptr peakWS = getProperty("OutputWorkspace");
  if (peakWS != inPeakWS) {
    peakWS = inPeakWS->clone();
  }

  {
    const SpecialCoordinateSystem mdCoordinates =
        mdWS->getSpecialCoordinateSystem();
    if (mdCoordinates == None) {
      throw std::invalid_argument("The coordinate system of the input "
                                  "MDWorkspace cannot be established. Create "
                                  "your workspace with an MDFrame which is "
                                  "not a General Frame or Unknown Frame.");
    }
  }

  const double threshold = getProperty("Threshold");
  // Make a background strategy for the CCL analysis to use.
  HardThresholdBackground backgroundStrategy(threshold,
                                             this->getNormalization());
  // CCL. Multi-processor version.
  ConnectedComponentLabeling analysis;

  Progress progress(this, 0.0, 1.0, 1);
  // Perform CCL.
  ClusterTuple clusters =
      analysis.executeAndFetchClusters(mdWS, &backgroundStrategy, progress);
  // Extract the clusters
  ConnectedComponentMappingTypes::ClusterMap &clusterMap = clusters.get<1>();
  // Extract the labeled image
  IMDHistoWorkspace_sptr outHistoWS = clusters.get<0>();
  // Labels taken by peaks.
  std::map<size_t, size_t> labelsTakenByPeaks;
  // Make a peak transform so that we can understand a peak in the context of
  // the mdworkspace coordinate setup.
  PeakClusterProjection projection(outHistoWS); // Projection of PeaksWorkspace
                                                // over labelled cluster
                                                // workspace.

  progress.doReport("Performing Peak Integration");
  g_log.information("Starting Integration");
  progress.resetNumSteps(peakWS->getNumberPeaks(), 0.9, 1);
  PARALLEL_FOR_IF(Kernel::threadSafe(*peakWS))
  for (int i = 0; i < peakWS->getNumberPeaks(); ++i) {
    PARALLEL_START_INTERUPT_REGION
    Geometry::IPeak &peak = peakWS->getPeak(i);
    const Mantid::signal_t signalValue = projection.signalAtPeakCenter(
        peak); // No normalization when extracting label ids!
    if (std::isnan(signalValue)) {
      g_log.warning()
          << "Warning: image for integration is off edge of detector for peak "
          << i << '\n';
    } else if (signalValue <
               static_cast<Mantid::signal_t>(analysis.getStartLabelId())) {
      g_log.information() << "Peak: " << i
                          << " Has no corresponding cluster/blob detected on "
                             "the image. This could be down to your Threshold "
                             "settings.\n";
    } else {
      const size_t labelIdAtPeak = static_cast<size_t>(signalValue);
      ICluster *const cluster = clusterMap[labelIdAtPeak].get();
      ICluster::ClusterIntegratedValues integratedValues =
          cluster->integrate(mdWS);
      peak.setIntensity(integratedValues.get<0>());
      peak.setSigmaIntensity(std::sqrt(integratedValues.get<1>()));

      PARALLEL_CRITICAL(IntegratePeaksUsingClusters) {
        auto it = labelsTakenByPeaks.find(labelIdAtPeak);
        if (it != labelsTakenByPeaks.end()) {
          g_log.warning() << "Overlapping Peaks. Peak: " << i
                          << " overlaps with another Peak: " << it->second
                          << " and shares label id: " << it->first << '\n';
        }
        labelsTakenByPeaks.emplace(labelIdAtPeak, i);
      }
      progress.report();
    }
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  setProperty("OutputWorkspace", peakWS);
  setProperty("OutputWorkspaceMD", outHistoWS);
}

} // namespace Crystal
} // namespace Mantid
