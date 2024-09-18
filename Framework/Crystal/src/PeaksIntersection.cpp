// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/PeaksIntersection.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/Crystal/IPeak.h"
#include "MantidKernel/ListValidator.h"

#include <boost/function.hpp>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using Mantid::DataObjects::Peak;
using Mantid::DataObjects::PeaksWorkspace;
using Mantid::DataObjects::PeaksWorkspace_sptr;

namespace Mantid::Crystal {
std::string PeaksIntersection::detectorSpaceFrame() { return "Detector space"; }

std::string PeaksIntersection::qLabFrame() { return "Q (lab frame)"; }

std::string PeaksIntersection::qSampleFrame() { return "Q (sample frame)"; }

std::string PeaksIntersection::hklFrame() { return "HKL"; }

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void PeaksIntersection::initBaseProperties() {
  declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>("InputWorkspace", "", Direction::Input),
                  "An input peaks workspace.");

  std::vector<std::string> propOptions;
  propOptions.emplace_back(detectorSpaceFrame());
  propOptions.emplace_back(qLabFrame());
  propOptions.emplace_back(qSampleFrame());
  propOptions.emplace_back(hklFrame());

  declareProperty("CoordinateFrame", "DetectorSpace", std::make_shared<StringListValidator>(propOptions),
                  "What coordinate system to use for intersection criteria?\n"
                  "  DetectorSpace: Real-space coordinates.\n"
                  "  Q (lab frame): Wave-vector change of the lattice in the lab frame.\n"
                  "  Q (sample frame): Momentum in the sample frame.\n"
                  "  HKL");

  declareProperty("PeakRadius", 0.0, "Effective peak radius in CoordinateFrame");

  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>("OutputWorkspace", "", Direction::Output),
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
  PeaksWorkspace_sptr ws = this->getProperty("InputWorkspace");

  m_peakRadius = this->getProperty("PeakRadius");

  boost::function<V3D(Peak *)> coordFrameFunc;
  coordFrameFunc = &Peak::getHKL;
  if (coordinateFrame == detectorSpaceFrame()) {
    coordFrameFunc = &Peak::getDetectorPosition;
  } else if (coordinateFrame == qLabFrame()) {
    coordFrameFunc = &Peak::getQLabFrame;
  } else if (coordinateFrame == qSampleFrame()) {
    coordFrameFunc = &Peak::getQSampleFrame;
  }

  // Create the faces.
  VecVecV3D faces = this->createFaces();

  const int nPeaks = ws->getNumberPeaks();
  const int facesN = this->numberOfFaces();

  // Calculate the normals for each face.
  VecV3D normals(facesN);
  for (int i = 0; i < facesN; ++i) {
    VecV3D face = faces[i];
    normals[i] = (face[1] - face[0]).cross_prod((face[2] - face[0]));
    const auto norm = normals[i].norm();
    if (norm == 0) {
      // Try to build a normal still perpendicular to the faces.
      const auto v = face[1] - face[0];
      const auto r = std::hypot(v.X(), v.Y());
      if (r == 0.) {
        normals[i] = V3D(1., 0., 0.);
      } else {
        const auto x = v.Y() / r;
        const auto y = std::sqrt(1 - x * x);
        normals[i] = V3D(x, y, 0.);
      }
    } else {
      normals[i] /= norm;
    }
  }

  Mantid::DataObjects::TableWorkspace_sptr outputWorkspace =
      std::make_shared<Mantid::DataObjects::TableWorkspace>(ws->rowCount());
  outputWorkspace->addColumn("int", "PeakIndex");
  outputWorkspace->addColumn("bool", "Intersecting");
  outputWorkspace->addColumn("double", "Distance");

  size_t frequency = nPeaks;
  if (frequency > 100) {
    frequency = nPeaks / 100;
  }
  Progress prog(this, 0.0, 1.0, 100);

  PARALLEL_FOR_IF(Kernel::threadSafe(*ws, *outputWorkspace))
  for (int i = 0; i < nPeaks; ++i) {
    PARALLEL_START_INTERRUPT_REGION
    Peak *peak = dynamic_cast<Peak *>(ws->getPeakPtr(i));
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
        for (int j = 0; j < facesN; ++j) {
          distance = normals[j].scalar_prod(faces[j][0] - peakCenter); // Distance between plane and peak center.
          if (m_peakRadius >= std::abs(distance))                      // Sphere passes through one
                                                                       // of the PLANES defined by
                                                                       // the box faces.
          {
            // Check that it is actually within the face boundaries.
            const V3D touchPoint = (normals[j] * distance) + peakCenter; // Vector equation of line give
                                                                         // touch point on plane.

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
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  setProperty("OutputWorkspace", outputWorkspace);
}

} // namespace Mantid::Crystal
