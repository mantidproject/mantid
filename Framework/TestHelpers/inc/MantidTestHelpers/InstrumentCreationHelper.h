#ifndef INSTRUMENTCREATIONHELPER_H_
#define INSTRUMENTCREATIONHELPER_H_

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"

namespace InstrumentCreationHelper {
using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;

void addFullInstrumentToWorkspace(MatrixWorkspace &workspace,
                                  bool includeMonitors, bool startYNegative,
                                  const std::string &instrumentName) {
  auto instrument = boost::make_shared<Instrument>(instrumentName);
  instrument->setReferenceFrame(
      boost::make_shared<ReferenceFrame>(Y, Z, Left, ""));
  workspace.setInstrument(instrument);

  const double pixelRadius(0.05);
  Object_sptr pixelShape = ComponentCreationHelper::createCappedCylinder(
      pixelRadius, 0.02, V3D(0.0, 0.0, 0.0), V3D(0., 1.0, 0.), "tube");

  const double detXPos(5.0);
  auto ndets = workspace.getNumberHistograms();
  if (includeMonitors)
    ndets -= 2;
  for (size_t i = 0; i < ndets; ++i) {
    std::ostringstream lexer;
    lexer << "pixel-" << i << ")";
    Detector *physicalPixel =
        new Detector(lexer.str(), workspace.getAxis(1)->spectraNo(i), pixelShape,
                     instrument.get());
    auto ycount(i);
    if (startYNegative)
      ycount -= 1;
    const double ypos = static_cast<double>(ycount) * 2.0 * pixelRadius;
    physicalPixel->setPos(detXPos, ypos, 0.0);
    instrument->add(physicalPixel);
    instrument->markAsDetector(physicalPixel);
    workspace.getSpectrum(i)->addDetectorID(physicalPixel->getID());
  }

  // Monitors last
  if (includeMonitors) // These occupy the last 2 spectra
  {
    Detector *monitor1 =
        new Detector("mon1", workspace.getAxis(1)->spectraNo(ndets), Object_sptr(),
                     instrument.get());
    monitor1->setPos(-9.0, 0.0, 0.0);
    instrument->add(monitor1);
    instrument->markAsMonitor(monitor1);

    Detector *monitor2 =
        new Detector("mon2", workspace.getAxis(1)->spectraNo(ndets) + 1,
                     Object_sptr(), instrument.get());
    monitor2->setPos(-2.0, 0.0, 0.0);
    instrument->add(monitor2);
    instrument->markAsMonitor(monitor2);
  }

  // Define a source and sample position
  // Define a source component
  ObjComponent *source = new ObjComponent(
      "moderator",
      ComponentCreationHelper::createSphere(0.1, V3D(0, 0, 0), "1"),
      instrument.get());
  source->setPos(V3D(-20, 0.0, 0.0));
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
  Component *chop_pos =
      new Component("chopper-position", Kernel::V3D(-10, 0, 0), instrument.get());
  instrument->add(chop_pos);
}
}

#endif /* INSTRUMENTCREATIONHELPER_H_ */
