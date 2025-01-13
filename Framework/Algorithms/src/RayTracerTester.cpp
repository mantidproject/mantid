// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/RayTracerTester.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Objects/InstrumentRayTracer.h"
#include "MantidKernel/V3D.h"

#include <cmath>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using Mantid::Geometry::IDetector_const_sptr;
using Mantid::Geometry::InstrumentRayTracer;

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(RayTracerTester)

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void RayTracerTester::init() {
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, ".xml"),
                  "The filename (including its full or relative path) of an "
                  "instrument definition file");
  declareProperty("NumAzimuth", 100, "Steps in azimuthal angles");
  declareProperty("NumZenith", 50, "Steps in zenith angles");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void RayTracerTester::exec() {
  auto alg = createChildAlgorithm("LoadEmptyInstrument", 0.0, 0.3, true);
  alg->setPropertyValue("Filename", getPropertyValue("Filename"));
  alg->executeAsChildAlg();

  MatrixWorkspace_sptr mws = alg->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace", mws);
  Workspace2D_sptr ws = std::dynamic_pointer_cast<Workspace2D>(mws);

  detid2index_map detTowi = ws->getDetectorIDToWorkspaceIndexMap();
  for (size_t i = 0; i < ws->getNumberHistograms(); i++)
    ws->mutableY(i)[0] = 0.0;

  int NumAzimuth = getProperty("NumAzimuth");
  int NumZenith = getProperty("NumZenith");
  Progress prog(this, 0.3, 1.0, NumAzimuth);
  InstrumentRayTracer tracker(ws->getInstrument());
  for (int iaz = 0; iaz < NumAzimuth; iaz++) {
    prog.report();
    double az = double(iaz) * M_PI * 2.0 / double(NumAzimuth);
    for (int iz = 0; iz < NumZenith; iz++) {
      const double zen = double(iz) * M_PI / double(NumZenith);
      const double x = cos(az);
      const double z = sin(az);
      const double y = cos(zen);
      V3D beam(x, y, z);
      beam.normalize();

      // Create a ray tracer
      tracker.traceFromSample(beam);
      IDetector_const_sptr det = tracker.getDetectorResult();
      if (det) {
        size_t wi = detTowi[det->getID()];
        g_log.information() << "Found detector " << det->getID() << '\n';
        ws->mutableY(wi)[0] = double(int(az * 57.3) * 1000 + int(iz));
      }
    }
  }
}

} // namespace Mantid::Algorithms
