/*WIKI*

 Integrates arbitrary shaped single crystal peaks defined on an
 [[MDHistoWorkspace]] using connected component analysis to determine
 regions of interest around each peak of the [[PeaksWorkspace]]. The output is
 an integrated [[PeaksWorkspace]] as well as an image
 containing the labels assigned to each cluster for diagnostic and visualisation
 purposes.

 This algorithm is very similar to, [[IntegratePeaksUsingClusters]] but breaks
 the integration into a series of local image domains rather than integrating an
 image in one-shot.
 The advantages of this approach are that you can locally define a background
 rather than using a global setting, and are therefore better able to capture
 the peak shape. A further advantage
 is that the memory requirement is reduced, given that [[MDHistoWorkspaces]] are
 generated in the region of the peak, and therefore high resolution can be
 achieved in the region of the peaks without
 an overall high n-dimensional image cost.

 Unlike [[IntegratePeaksUsingClusters]] you do not need to specify at Threshold
 for background detection. You do however need to specify a
 BackgroundOuterRadius in a similar fashion to
 [[IntegratePeaksMD]]. This is used to determine the region in which to make an
 [[MDHistoWorkspace]] around each peak. A liberal estimate is a good idea.

 At present, you will need to provide NumberOfBins for binning (via [[BinMD]]
 axis-aligned). By default, the algorithm will create a 20 by 20 by 20 grid
 around each peak. The same
 number of bins is applied in each dimension.

 == Warnings and Logging ==
 See [[IntegratePeaksUsingClusters]] for notes on logs and warnings.

 *WIKI*/

#include "MantidCrystal/IntegratePeaksHybrid.h"

#include "MantidCrystal/ConnectedComponentLabeling.h"
#include "MantidCrystal/HardThresholdBackground.h"
#include "MantidCrystal/ICluster.h"
#include "MantidCrystal/PeakClusterProjection.h"

#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidDataObjects/PeaksWorkspace.h"

#include <boost/make_shared.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
#include <cmath>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

namespace {
/**
 * Create a BinMD dimension input string
 * @param dimension : Dimension to use
 * @param min : Min extents
 * @param max : Max extents
 * @param nBins : Number of bins.
 * @return
 */
std::string extractFormattedPropertyFromDimension(
    Mantid::Geometry::IMDDimension_const_sptr &dimension, const double &min,
    const double &max, const int &nBins) {
  std::string id = dimension->getDimensionId();
  return boost::str(boost::format("%s, %f, %f, %d") % id % min % max % nBins);
}
}

namespace Mantid {
namespace Crystal {
using namespace ConnectedComponentMappingTypes;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(IntegratePeaksHybrid)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
IntegratePeaksHybrid::IntegratePeaksHybrid() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
IntegratePeaksHybrid::~IntegratePeaksHybrid() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string IntegratePeaksHybrid::name() const {
  return "IntegratePeaksHybrid";
};

/// Algorithm's version for identification. @see Algorithm::version
int IntegratePeaksHybrid::version() const { return 1; };

/// Algorithm's category for identification. @see Algorithm::category
const std::string IntegratePeaksHybrid::category() const {
  return "MDAlgorithms";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void IntegratePeaksHybrid::init() {
  declareProperty(new WorkspaceProperty<IMDEventWorkspace>("InputWorkspace", "",
                                                           Direction::Input),
                  "Input md workspace.");
  declareProperty(new WorkspaceProperty<IPeaksWorkspace>("PeaksWorkspace", "",
                                                         Direction::Input),
                  "A PeaksWorkspace containing the peaks to integrate.");

  auto positiveIntValidator = boost::make_shared<BoundedValidator<int>>();
  positiveIntValidator->setExclusive(true);
  positiveIntValidator->setLower(0);

  declareProperty(new PropertyWithValue<int>("NumberOfBins", 20,
                                             positiveIntValidator,
                                             Direction::Input),
                  "Number of bins to use while creating each local image. "
                  "Defaults to 20. Increase to reduce pixelation");

  auto compositeValidator = boost::make_shared<CompositeValidator>();
  auto positiveDoubleValidator = boost::make_shared<BoundedValidator<double>>();
  positiveDoubleValidator->setExclusive(true);
  positiveDoubleValidator->setLower(0);
  compositeValidator->add(positiveDoubleValidator);
  compositeValidator->add(boost::make_shared<MandatoryValidator<double>>());

  declareProperty(new PropertyWithValue<double>("BackgroundOuterRadius", 0.0,
                                                compositeValidator,
                                                Direction::Input),
                  "Background outer radius estimate. Choose liberal value.");

  declareProperty(new WorkspaceProperty<IPeaksWorkspace>("OutputWorkspace", "",
                                                         Direction::Output),
                  "An output integrated peaks workspace.");

  declareProperty(new WorkspaceProperty<WorkspaceGroup>("OutputWorkspaces", "",
                                                        Direction::Output),
                  "MDHistoWorkspaces containing the labeled clusters used by "
                  "the algorithm.");
}

const std::string IntegratePeaksHybrid::summary() const {
  return "Integrate single crystal peaks using connected component analysis. "
         "Binning invididual to each peak.";
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void IntegratePeaksHybrid::exec() {
  IMDEventWorkspace_sptr mdWS = getProperty("InputWorkspace");
  IPeaksWorkspace_sptr inPeakWS = getProperty("PeaksWorkspace");
  IPeaksWorkspace_sptr peakWS = getProperty("OutputWorkspace");
  const int numBins = getProperty("NumberOfBins");
  const double peakOuterRadius = getProperty("BackgroundOuterRadius");
  const double halfPeakOuterRadius = peakOuterRadius / 2;
  if (peakWS != inPeakWS) {
    peakWS = IPeaksWorkspace_sptr(
        dynamic_cast<IPeaksWorkspace *>(inPeakWS->clone()));
  }

  {
    const SpecialCoordinateSystem mdCoordinates =
        mdWS->getSpecialCoordinateSystem();
    if (mdCoordinates == None) {
      throw std::invalid_argument("The coordinate system of the input "
                                  "MDWorkspace cannot be established. Run "
                                  "SetSpecialCoordinates on InputWorkspace.");
    }
  }

  PeakClusterProjection projection(mdWS);
  auto outImageResults = boost::make_shared<WorkspaceGroup>();

  Progress progress(this, 0, 1, peakWS->getNumberPeaks());

  PARALLEL_FOR1(peakWS)
  for (int i = 0; i < peakWS->getNumberPeaks(); ++i) {

    PARALLEL_START_INTERUPT_REGION
    IPeak &peak = peakWS->getPeak(i);
    const V3D center = projection.peakCenter(peak);

    auto binMDAlg = this->createChildAlgorithm("BinMD");
    binMDAlg->setProperty("InputWorkspace", mdWS);
    binMDAlg->setPropertyValue("OutputWorkspace", "output_ws");
    binMDAlg->setProperty("AxisAligned", true);

    for (int j = 0; j < static_cast<int>(mdWS->getNumDims()); ++j) {
      std::stringstream propertyName;
      propertyName << "AlignedDim" << j;

      auto dimension = mdWS->getDimension(j);

      double min = center[j] - halfPeakOuterRadius;
      double max = center[j] + halfPeakOuterRadius;

      binMDAlg->setPropertyValue(
          propertyName.str(),
          extractFormattedPropertyFromDimension(dimension, min, max, numBins));
    }
    binMDAlg->execute();
    Workspace_sptr temp = binMDAlg->getProperty("OutputWorkspace");
    IMDHistoWorkspace_sptr localImage =
        boost::dynamic_pointer_cast<IMDHistoWorkspace>(temp);
    API::MDNormalization normalization = NoNormalization;
    auto iterator = localImage->createIterator();
    iterator->setNormalization(normalization);
    signal_t cumulative = 0;
    do {
      cumulative += iterator->getSignal();
    } while (iterator->next());
    const double threshold = cumulative / signal_t(localImage->getNPoints());

    HardThresholdBackground backgroundStrategy(threshold, normalization);
    // CCL. Multi-processor version.
    const size_t startId = 1;
    const size_t nThreads = 1;
    ConnectedComponentLabeling analysis(
        startId, nThreads); // CCL executed single threaded.

    Progress dummyProgress;
    // Perform CCL.
    ClusterTuple clusters = analysis.executeAndFetchClusters(
        localImage, &backgroundStrategy, dummyProgress);
    // Extract the clusters
    ConnectedComponentMappingTypes::ClusterMap &clusterMap = clusters.get<1>();
    // Extract the labeled image
    IMDHistoWorkspace_sptr outHistoWS = clusters.get<0>();
    outImageResults->addWorkspace(outHistoWS);

    PeakClusterProjection localProjection(outHistoWS);
    const Mantid::signal_t signalValue = localProjection.signalAtPeakCenter(
        peak); // No normalization when extracting label ids!

    if (boost::math::isnan(signalValue)) {
      g_log.warning()
          << "Warning: image for integration is off edge of detector for peak "
          << i << std::endl;
    } else if (signalValue <
               static_cast<Mantid::signal_t>(analysis.getStartLabelId())) {
      g_log.information() << "Peak: " << i
                          << " Has no corresponding cluster/blob detected on "
                             "the image. This could be down to your Threshold "
                             "settings." << std::endl;
    } else {
      const size_t labelIdAtPeak = static_cast<size_t>(signalValue);
      ICluster *const cluster = clusterMap[labelIdAtPeak].get();
      ICluster::ClusterIntegratedValues integratedValues =
          cluster->integrate(localImage);
      peak.setIntensity(integratedValues.get<0>());
      peak.setSigmaIntensity(std::sqrt(integratedValues.get<1>()));
    }
    progress.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  setProperty("OutputWorkspace", peakWS);
  setProperty("OutputWorkspaces", outImageResults);
}

} // namespace Crystal
} // namespace Mantid
