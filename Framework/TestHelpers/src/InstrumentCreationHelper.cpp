// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidTestHelpers/InstrumentCreationHelper.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"

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
  auto pixelShape = ComponentCreationHelper::createCappedCylinder(
      pixelRadius, 0.02, V3D(0.0, 0.0, 0.0), V3D(0., 1.0, 0.), "tube");

  const double detZPos(5.0);
  // Careful! Do not use size_t or auto, the unsigned will break the -=2 below.
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
                     IObject_sptr(), instrument.get());
    monitor1->setPos(0.0, 0.0, -9.0);
    instrument->add(monitor1);
    instrument->markAsMonitor(monitor1);
    workspace.getSpectrum(ndets).setDetectorID(ndets + 1);

    Detector *monitor2 =
        new Detector("mon2", workspace.getAxis(1)->spectraNo(ndets) + 1,
                     IObject_sptr(), instrument.get());
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

/** Adds a component to an instrument
 *
 * @param instrument :: instrument to which the component will be added
 * @param position :: position of the component
 * @param name :: name of the component
 * @return a component pointer
 */
ObjComponent *addComponent(Mantid::Geometry::Instrument_sptr &instrument,
                           const Mantid::Kernel::V3D &position,
                           const std::string &name) {
  ObjComponent *component = new ObjComponent(name);
  component->setPos(position);
  instrument->add(component);
  return component;
}

/** Adds a sample to an instrument
 *
 * @param instrument :: instrument to which the sample will be added
 * @param position :: position of the sample
 * @param name :: name of the sample
 */
void addSample(Mantid::Geometry::Instrument_sptr &instrument,
               const Mantid::Kernel::V3D &position, const std::string &name) {
  auto sample = addComponent(instrument, position, name);
  instrument->markAsSamplePos(sample);
}

/** Adds a source to an instrument
 *
 * @param instrument :: instrument to which the source will be added
 * @param position :: position of the source
 * @param name :: name of the source
 */
void addSource(Mantid::Geometry::Instrument_sptr &instrument,
               const Mantid::Kernel::V3D &position, const std::string &name) {
  auto source = addComponent(instrument, position, name);
  instrument->markAsSource(source);
}

/** Adds a monitor to an instrument
 *
 * @param instrument :: instrument to which the monitor will be added
 * @param position :: position of the monitor
 * @param ID :: identification number of the monitor
 * @param name :: name of the monitor
 */
void addMonitor(Mantid::Geometry::Instrument_sptr &instrument,
                const Mantid::Kernel::V3D &position, const int ID,
                const std::string &name) {
  Detector *monitor = new Detector(name, ID, nullptr);
  monitor->setPos(position);
  instrument->add(monitor);
  instrument->markAsMonitor(monitor);
}

/** Adds a detector to an instrument
 *
 * @param instrument :: instrument to which the detector will be added
 * @param position :: position of the detector
 * @param ID :: identification number of the detector
 * @param name :: name of the detector
 */
void addDetector(Mantid::Geometry::Instrument_sptr &instrument,
                 const Mantid::Kernel::V3D &position, const int ID,
                 const std::string &name) {
  // Where 0.01 is half detector width etc.
  Detector *detector = new Detector(
      name, ID, ComponentCreationHelper::createCuboid(0.01, 0.02, 0.03),
      nullptr);
  detector->setPos(position);
  instrument->add(detector);
  instrument->markAsDetector(detector);
}
} // namespace InstrumentCreationHelper
