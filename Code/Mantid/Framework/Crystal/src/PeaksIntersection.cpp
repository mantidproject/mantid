#include "MantidKernel/ListValidator.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/IPeak.h"
#include "MantidAPI/TableRow.h"
#include "MantidCrystal/PeaksIntersection.h"
#include "MantidDataObjects/TableWorkspace.h"

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace Mantid {
namespace Crystal {
std::string PeaksIntersection::detectorSpaceFrame() { return "Detector space"; }

std::string PeaksIntersection::qLabFrame() { return "Q (lab frame)"; }

std::string PeaksIntersection::qSampleFrame() { return "Q (sample frame)"; }

std::string PeaksIntersection::hklFrame() { return "HKL"; }

//----------------------------------------------------------------------------------------------
/** Constructor
 */
PeaksIntersection::PeaksIntersection() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
PeaksIntersection::~PeaksIntersection() {}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void PeaksIntersection::initBaseProperties() {
  declareProperty(new WorkspaceProperty<IPeaksWorkspace>("InputWorkspace", "",
                                                         Direction::Input),
                  "An input peaks workspace.");

  std::vector<std::string> propOptions;
  propOptions.push_back(detectorSpaceFrame());
  propOptions.push_back(qLabFrame());
  propOptions.push_back(qSampleFrame());
  propOptions.push_back(hklFrame());

  declareProperty(
      "CoordinateFrame", "DetectorSpace",
      boost::make_shared<StringListValidator>(propOptions),
      "What coordinate system to use for intersection criteria?\n"
      "  DetectorSpace: Real-space coordinates.\n"
      "  Q (lab frame): Wave-vector change of the lattice in the lab frame.\n"
      "  Q (sample frame): Momentum in the sample frame.\n"
      "  HKL");

  declareProperty("PeakRadius", 0.0,
                  "Effective peak radius in CoordinateFrame");

  declareProperty(new WorkspaceProperty<ITableWorkspace>("OutputWorkspace", "",
                                                         Direction::Output),
                  "An output table workspace. Two columns. Peak index into "
                  "input workspace, and boolean, where true is for positive "
                  "intersection.");
}

/**
Getter for the peak radius.
*/
double PeaksIntersection::getPeakRadius() const { return m_peakRadius; }

/**
Run the algorithm.
@param checkPeakExtents : If set true, checks the peak radius interaction with
the surfaces.
*/
void PeaksIntersection::executePeaksIntersection(const bool checkPeakExtents) {
  const std::string coordinateFrame = this->getPropertyValue("CoordinateFrame");
  IPeaksWorkspace_sptr ws = this->getProperty("InputWorkspace");

  m_peakRadius = this->getProperty("PeakRadius");

  // Find the coordinate frame to use an set up boost function for this.
  boost::function<V3D(IPeak *)> coordFrameFunc = &IPeak::getHKL;
  if (coordinateFrame == detectorSpaceFrame()) {
    coordFrameFunc = &IPeak::getDetectorPosition;
  } else if (coordinateFrame == qLabFrame()) {
    coordFrameFunc = &IPeak::getQLabFrame;
  } else if (coordinateFrame == qSampleFrame()) {
    coordFrameFunc = &IPeak::getQSampleFrame;
  }

  // Create the faces.
  VecVecV3D faces = this->createFaces();

  const int nPeaks = ws->getNumberPeaks();
  const int numberOfFaces = this->numberOfFaces();

  // Calculate the normals for each face.
  VecV3D normals(numberOfFaces);
  for (int i = 0; i < numberOfFaces; ++i) {
    VecV3D face = faces[i];
    normals[i] = (face[1] - face[0]).cross_prod((face[2] - face[0]));
    normals[i].normalize();
  }

  Mantid::DataObjects::TableWorkspace_sptr outputWorkspace =
      boost::make_shared<Mantid::DataObjects::TableWorkspace>(ws->rowCount());
  outputWorkspace->addColumn("int", "PeakIndex");
  outputWorkspace->addColumn("bool", "Intersecting");
  outputWorkspace->addColumn("double", "Distance");

  size_t frequency = nPeaks;
  if (frequency > 100) {
    frequency = nPeaks / 100;
  }
  Progress prog(this, 0, 1, 100);

  PARALLEL_FOR2(ws, outputWorkspace)
  for (int i = 0; i < nPeaks; ++i) {
    PARALLEL_START_INTERUPT_REGION
    IPeak *peak = ws->getPeakPtr(i);
    V3D peakCenter = coordFrameFunc(peak);

    if (i % frequency == 0)
      prog.report();

    bool doesIntersect = true;
    double distance = 0;
    if (pointOutsideAnyExtents(peakCenter)) {
      // Out of bounds.
      doesIntersect = false;

      if (checkPeakExtents) {
        // Take account of radius spherical extents.
        for (int i = 0; i < numberOfFaces; ++i) {
          distance = normals[i].scalar_prod(
              faces[i][0] -
              peakCenter); // Distance between plane and peak center.
          if (m_peakRadius >= std::abs(distance)) // Sphere passes through one
                                                  // of the PLANES defined by
                                                  // the box faces.
          {
            // Check that it is actually within the face boundaries.
            V3D touchPoint = (normals[i] * distance) +
                             peakCenter; // Vector equation of line give touch
                                         // point on plane.

            // checkTouchPoint(touchPoint, normals[i], faces[i][0]); //
            // Debugging line.

            if (pointInsideAllExtents(touchPoint, peakCenter)) {
              doesIntersect = true;
              break;
            }
          }
        }
      }
    }

    TableRow row = outputWorkspace->getRow(i);
    row << i << doesIntersect << distance;
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  setProperty("OutputWorkspace", outputWorkspace);
}

} // namespace Crystal
} // namespace Mantid