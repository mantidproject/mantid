#include "MantidKernel/System.h"
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/CoordTransformDistance.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidMDAlgorithms/IntegratePeaksMD.h"
#include "MantidMDAlgorithms/CentroidPeaksMD2.h"

using Mantid::DataObjects::PeaksWorkspace;

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CentroidPeaksMD2)

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CentroidPeaksMD2::init() {
  declareProperty(make_unique<WorkspaceProperty<IMDEventWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "An input MDEventWorkspace.");

  declareProperty(
      make_unique<PropertyWithValue<double>>("PeakRadius", 1.0,
                                             Direction::Input),
      "Fixed radius around each peak position in which to calculate the "
      "centroid.");

  declareProperty(make_unique<WorkspaceProperty<PeaksWorkspace>>(
                      "PeaksWorkspace", "", Direction::Input),
                  "A PeaksWorkspace containing the peaks to centroid.");

  declareProperty(
      make_unique<WorkspaceProperty<PeaksWorkspace>>("OutputWorkspace", "",
                                                     Direction::Output),
      "The output PeaksWorkspace will be a copy of the input PeaksWorkspace "
      "with the peaks' positions modified by the new found centroids.");
}

//----------------------------------------------------------------------------------------------
/** Integrate the peaks of the workspace using parameters saved in the algorithm
 * class
 * @param ws ::  MDEventWorkspace to integrate
 */
template <typename MDE, size_t nd>
void CentroidPeaksMD2::integrate(typename MDEventWorkspace<MDE, nd>::sptr ws) {
  if (nd != 3)
    throw std::invalid_argument("For now, we expect the input MDEventWorkspace "
                                "to have 3 dimensions only.");

  /// Peak workspace to centroid
  Mantid::DataObjects::PeaksWorkspace_sptr inPeakWS =
      getProperty("PeaksWorkspace");

  /// Output peaks workspace, create if needed
  Mantid::DataObjects::PeaksWorkspace_sptr peakWS =
      getProperty("OutputWorkspace");

  if (peakWS != inPeakWS)
    peakWS = inPeakWS->clone();

  int CoordinatesToUse = ws->getSpecialCoordinateSystem();

  /// Radius to use around peaks
  double PeakRadius = getProperty("PeakRadius");

  // cppcheck-suppress syntaxError
    PRAGMA_OMP(parallel for schedule(dynamic, 10) )
    for (int i = 0; i < int(peakWS->getNumberPeaks()); ++i) {
      // Get a direct ref to that peak.
      IPeak &p = peakWS->getPeak(i);
      double detectorDistance = p.getL2();

      // Get the peak center as a position in the dimensions of the workspace
      V3D pos;
      if (CoordinatesToUse == 1) //"Q (lab frame)"
        pos = p.getQLabFrame();
      else if (CoordinatesToUse == 2) //"Q (sample frame)"
        pos = p.getQSampleFrame();
      else if (CoordinatesToUse == 3) //"HKL"
        pos = p.getHKL();

      // Build the sphere transformation
      bool dimensionsUsed[nd];
      coord_t center[nd];
      for (size_t d = 0; d < nd; ++d) {
        dimensionsUsed[d] = true; // Use all dimensions
        center[d] = static_cast<coord_t>(pos[d]);
      }
      CoordTransformDistance sphere(nd, center, dimensionsUsed);

      // Initialize the centroid to 0.0
      signal_t signal = 0;
      coord_t centroid[nd];
      for (size_t d = 0; d < nd; d++)
        centroid[d] = 0.0;

      // Perform centroid
      ws->getBox()->centroidSphere(
          sphere, static_cast<coord_t>(PeakRadius * PeakRadius), centroid,
          signal);

      // Normalize by signal
      if (signal != 0.0) {
        for (size_t d = 0; d < nd; d++)
          centroid[d] /= static_cast<coord_t>(signal);

        V3D vecCentroid(centroid[0], centroid[1], centroid[2]);

        // Save it back in the peak object, in the dimension specified.
        try {
          if (CoordinatesToUse == 1) //"Q (lab frame)"
          {
            p.setQLabFrame(vecCentroid, detectorDistance);
            p.findDetector();
          } else if (CoordinatesToUse == 2) //"Q (sample frame)"
          {
            p.setQSampleFrame(vecCentroid, detectorDistance);
            p.findDetector();
          } else if (CoordinatesToUse == 3) //"HKL"
          {
            p.setHKL(vecCentroid);
          }
        } catch (std::exception &e) {
          g_log.warning() << "Error setting Q or HKL\n";
          g_log.warning() << e.what() << '\n';
        }

        g_log.information() << "Peak " << i << " at " << pos << ": signal "
                            << signal << ", centroid " << vecCentroid << " in "
                            << CoordinatesToUse << '\n';
      } else {
        g_log.information() << "Peak " << i << " at " << pos
                            << " had no signal, and could not be centroided.\n";
      }
    }

    // Save the output
    setProperty("OutputWorkspace", peakWS);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CentroidPeaksMD2::exec() {
  inWS = getProperty("InputWorkspace");

  CALL_MDEVENT_FUNCTION3(this->integrate, inWS);
}

} // namespace Mantid
} // namespace DataObjects
