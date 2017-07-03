#include "MantidTestHelpers/InstrumentCreationHelper.h"

#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Axis.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;
namespace InstrumentCreationHelper {

void addFullInstrumentToWorkspace(MatrixWorkspace &workspace,
                                  bool includeMonitors, bool startYNegative,
                                  const std::string &instrumentName) {
  auto instrument = boost::make_shared<Instrument>(instrumentName);
  instrument->setReferenceFrame(
      boost::make_shared<ReferenceFrame>(Y, Z, Right, ""));

  const double pixelRadius(0.05);
  Object_sptr pixelShape = ComponentCreationHelper::createCappedCylinder(
      pixelRadius, 0.02, V3D(0.0, 0.0, 0.0), V3D(0., 1.0, 0.), "tube");

  const double detZPos(5.0);
  // Careful! Do not use size_t or auto, the unisgned will break the -=2 below.
  int ndets = static_cast<int>(workspace.getNumberHistograms());
  if (includeMonitors)
    ndets -= 2;
  for (int i = 0; i < ndets; ++i) {
    std::ostringstream lexer;
    lexer << "pixel-" << i << ")";
    Detector *physicalPixel =
        new Detector(lexer.str(), workspace.getAxis(1)->spectraNo(i),
                     pixelShape, instrument.get());
    int ycount(i);
    if (startYNegative)
      ycount -= 1;
    const double ypos = ycount * 2.0 * pixelRadius;
    physicalPixel->setPos(0.0, ypos, detZPos);
    instrument->add(physicalPixel);
    instrument->markAsDetector(physicalPixel);
    workspace.getSpectrum(i).setDetectorID(physicalPixel->getID());
  }

  // Monitors last
  if (includeMonitors) // These occupy the last 2 spectra
  {
    Detector *monitor1 =
        new Detector("mon1", workspace.getAxis(1)->spectraNo(ndets),
                     Object_sptr(), instrument.get());
    monitor1->setPos(0.0, 0.0, -9.0);
    instrument->add(monitor1);
    instrument->markAsMonitor(monitor1);
    workspace.getSpectrum(ndets).setDetectorID(ndets + 1);

    Detector *monitor2 =
        new Detector("mon2", workspace.getAxis(1)->spectraNo(ndets) + 1,
                     Object_sptr(), instrument.get());
    monitor2->setPos(0.0, 0.0, -2.0);
    instrument->add(monitor2);
    instrument->markAsMonitor(monitor2);
    workspace.getSpectrum(ndets + 1).setDetectorID(ndets + 2);
  }

  // Define a source and sample position
  // Define a source component
  ObjComponent *source = new ObjComponent(
      "moderator",
      ComponentCreationHelper::createSphere(0.1, V3D(0, 0, 0), "1"),
      instrument.get());
  source->setPos(V3D(0.0, 0.0, -20.0));
  instrument->add(source);
  instrument->markAsSource(source);

  // Define a sample as a simple sphere
  ObjComponent *sample = new ObjComponent(
      "samplePos",
      ComponentCreationHelper::createSphere(0.1, V3D(0, 0, 0), "1"),
      instrument.get());
  instrument->setPos(0.0, 0.0, 0.0);
  instrument->add(sample);
  instrument->markAsSamplePos(sample);
  // chopper position
  Component *chop_pos = new Component("chopper-position",
                                      Kernel::V3D(0, 0, -10), instrument.get());
  instrument->add(chop_pos);
  workspace.setInstrument(instrument);
}
}
