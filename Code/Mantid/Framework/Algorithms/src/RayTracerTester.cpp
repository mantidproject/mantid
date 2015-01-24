#include "MantidAlgorithms/RayTracerTester.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Objects/InstrumentRayTracer.h"
#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using Mantid::Geometry::IDetector_const_sptr;
using Mantid::Geometry::InstrumentRayTracer;

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(RayTracerTester)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
RayTracerTester::RayTracerTester() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
RayTracerTester::~RayTracerTester() {}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void RayTracerTester::init() {
  declareProperty(new FileProperty("Filename", "", FileProperty::Load, ".xml"),
                  "The filename (including its full or relative path) of an "
                  "instrument definition file");
  declareProperty("NumAzimuth", 100, "Steps in azimuthal angles");
  declareProperty("NumZenith", 50, "Steps in zenith angles");
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void RayTracerTester::exec() {
  IAlgorithm_sptr alg =
      this->createChildAlgorithm("LoadEmptyInstrument", 0.0, 0.3, true);
  alg->setPropertyValue("Filename", getPropertyValue("Filename"));
  alg->executeAsChildAlg();

  MatrixWorkspace_sptr mws = alg->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace", mws);
  Workspace2D_sptr ws = boost::dynamic_pointer_cast<Workspace2D>(mws);

  detid2index_map detTowi = ws->getDetectorIDToWorkspaceIndexMap();
  for (size_t i = 0; i < ws->getNumberHistograms(); i++)
    ws->dataY(i)[0] = 0.0;

  int NumAzimuth = getProperty("NumAzimuth");
  int NumZenith = getProperty("NumZenith");
  Progress prog(this, 0.3, 1.0, NumAzimuth);
  for (int iaz = 0; iaz < NumAzimuth; iaz++) {
    prog.report();
    double az = double(iaz) * 3.14159 * 2.0 / double(NumAzimuth);
    for (int iz = 0; iz < NumZenith; iz++) {
      double zen = double(iz) * 3.14159 * 1.0 / double(NumZenith);
      double x = cos(az);
      double z = sin(az);
      double y = cos(zen);
      V3D beam(x, y, z);

      // Create a ray tracer
      InstrumentRayTracer tracker(ws->getInstrument());
      tracker.traceFromSample(beam);
      IDetector_const_sptr det = tracker.getDetectorResult();
      if (det) {
        size_t wi = detTowi[det->getID()];
        g_log.information() << "Found detector " << det->getID() << std::endl;
        ws->dataY(wi)[0] = double(int(az * 57.3) * 1000 + int(iz));
      }
    }
  }
}

} // namespace Mantid
} // namespace Algorithms
