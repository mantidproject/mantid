// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/CentroidPeaksMD.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/CoordTransformDistance.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/ListValidator.h"
#include "MantidMDAlgorithms/IntegratePeaksMD.h"

using Mantid::DataObjects::PeaksWorkspace;

namespace Mantid::MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CentroidPeaksMD)

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

CentroidPeaksMD::CentroidPeaksMD() { this->useAlgorithm("CentroidPeaksMD", 2); }

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CentroidPeaksMD::init() {
  declareProperty(std::make_unique<WorkspaceProperty<IMDEventWorkspace>>("InputWorkspace", "", Direction::Input),
                  "An input MDEventWorkspace.");

  std::vector<std::string> propOptions{"Q (lab frame)", "Q (sample frame)", "HKL"};
  declareProperty("CoordinatesToUse", "HKL", std::make_shared<StringListValidator>(propOptions),
                  "Ignored:  algorithm uses the InputWorkspace's coordinates.");

  declareProperty(std::make_unique<PropertyWithValue<double>>("PeakRadius", 1.0, Direction::Input),
                  "Fixed radius around each peak position in which to calculate the "
                  "centroid.");

  declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>("PeaksWorkspace", "", Direction::Input),
                  "A PeaksWorkspace containing the peaks to centroid.");

  declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "The output PeaksWorkspace will be a copy of the input PeaksWorkspace "
                  "with the peaks' positions modified by the new found centroids.");
}

//----------------------------------------------------------------------------------------------
/** Integrate the peaks of the workspace using parameters saved in the algorithm
 * class
 * @param ws ::  MDEventWorkspace to integrate
 */
template <typename MDE, size_t nd> void CentroidPeaksMD::integrate(typename MDEventWorkspace<MDE, nd>::sptr ws) {
  if (nd != 3)
    throw std::invalid_argument("For now, we expect the input MDEventWorkspace "
                                "to have 3 dimensions only.");

  /// Peak workspace to centroid
  Mantid::DataObjects::PeaksWorkspace_sptr inPeakWS = getProperty("PeaksWorkspace");

  /// Output peaks workspace, create if needed
  Mantid::DataObjects::PeaksWorkspace_sptr peakWS = getProperty("OutputWorkspace");
  if (peakWS != inPeakWS)
    peakWS = inPeakWS->clone();

  std::string CoordinatesToUseStr = getPropertyValue("CoordinatesToUse");
  int CoordinatesToUse = ws->getSpecialCoordinateSystem();
  if (CoordinatesToUse == 1 && CoordinatesToUseStr != "Q (lab frame)")
    g_log.warning() << "Warning: used Q (lab frame) coordinates for MD "
                       "workspace, not CoordinatesToUse from input \n";
  else if (CoordinatesToUse == 2 && CoordinatesToUseStr != "Q (sample frame)")
    g_log.warning() << "Warning: used Q (sample frame) coordinates for MD "
                       "workspace, not CoordinatesToUse from input \n";
  else if (CoordinatesToUse == 3 && CoordinatesToUseStr != "HKL")
    g_log.warning() << "Warning: used HKL coordinates for MD workspace, not "
                       "CoordinatesToUse from input \n";

  /// Radius to use around peaks
  double PeakRadius = getProperty("PeakRadius");

    PRAGMA_OMP(parallel for schedule(dynamic, 10) )
    for (int i = 0; i < int(peakWS->getNumberPeaks()); ++i) {
      Peak &p = peakWS->getPeak(i);
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
      ws->getBox()->centroidSphere(sphere, static_cast<coord_t>(PeakRadius * PeakRadius), centroid, signal);

      // Normalize by signal
      if (signal != 0.0) {
        for (size_t d = 0; d < nd; d++)
          centroid[d] /= static_cast<coord_t>(signal);

        V3D vecCentroid(centroid[0], centroid[1], centroid[2]);

        // Save it back in the peak object, in the dimension specified.
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

        g_log.information() << "Peak " << i << " at " << pos << ": signal " << signal << ", centroid " << vecCentroid
                            << " in " << CoordinatesToUse << '\n';
      } else {
        g_log.information() << "Peak " << i << " at " << pos << " had no signal, and could not be centroided.\n";
      }
    }

    // Save the output
    setProperty("OutputWorkspace", peakWS);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CentroidPeaksMD::exec() {
  inWS = getProperty("InputWorkspace");

  CALL_MDEVENT_FUNCTION3(this->integrate, inWS);
}

} // namespace Mantid::MDAlgorithms
