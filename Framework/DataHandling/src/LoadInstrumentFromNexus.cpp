//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadInstrumentFromNexus.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidNexus/MuonNexusReader.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/FileProperty.h"

#include <fstream>

namespace Mantid {
namespace DataHandling {

DECLARE_ALGORITHM(LoadInstrumentFromNexus)

using namespace Kernel;
using namespace API;
using Geometry::Instrument;

/// Empty default constructor
LoadInstrumentFromNexus::LoadInstrumentFromNexus() {}

/// Initialisation method.
void LoadInstrumentFromNexus::init() {
  // When used as a Child Algorithm the workspace name is not used - hence the
  // "Anonymous" to satisfy the validator
  declareProperty(
      new WorkspaceProperty<MatrixWorkspace>("Workspace", "Anonymous",
                                             Direction::InOut),
      "The name of the workspace in which to attach the imported instrument");

  declareProperty(
      new FileProperty("Filename", "", FileProperty::Load, ".nxs"),
      "The name (including its full or relative path) of the Nexus file to "
      "attempt to load the instrument from. The file extension must either be "
      ".nxs or .NXS");
}

/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 *
 *  @throw FileError Thrown if unable to parse XML file
 */
void LoadInstrumentFromNexus::exec() {
  // Retrieve the filename from the properties
  m_filename = getPropertyValue("Filename");

  // Get the input workspace
  const MatrixWorkspace_sptr localWorkspace = getProperty("Workspace");

  // open Nexus file
  MuonNexusReader nxload;
  nxload.readFromFile(m_filename);
  progress(0.5);
  // Create a new Instrument with the right name and add it to the workspace
  Geometry::Instrument_sptr instrument(
      new Geometry::Instrument(nxload.getInstrumentName()));
  localWorkspace->setInstrument(instrument);

  // Add dummy source and samplepos to instrument
  // The L2 and 2-theta values from nexus file assumed to be relative to sample
  // position

  Geometry::ObjComponent *samplepos =
      new Geometry::ObjComponent("Unknown", instrument.get());
  instrument->add(samplepos);
  instrument->markAsSamplePos(samplepos);
  samplepos->setPos(0.0, 0.0, 0.0);

  Geometry::ObjComponent *source =
      new Geometry::ObjComponent("Unknown", instrument.get());
  instrument->add(source);
  instrument->markAsSource(source);
  double l1 = 0.0;
  // If user has provided an L1, use that
  if (!Kernel::ConfigService::Instance().getValue("instrument.L1", l1)) {
    // Otherwise try and get it from the nexus file - but not there at present!
    // l1 = nxload.ivpb.i_l1;
    // Default to 10 if the file doesn't have it set
    if (l1 == 0)
      l1 = 10.0;
  }
  source->setPos(0.0, -1.0 * l1, 0.0);
  progress(1.0);

  // add detectors
  /* **** Ignoring all this for the moment - the sample Nexus files do not
  contain most of these values

  const int numDetector = nxload.i_det;    // number of detectors
  const int* const detID = nxload.udet;    // detector IDs
  const float* const r = nxload.len2;      // distance from sample
  const float* const angle = nxload.tthe;  // angle between indicent beam and
  direction from sample to detector (two-theta)

  for (int i = 0; i < numDetector; ++i)
  {
    // Create a new detector. Instrument will take ownership of pointer so no
  need to delete.
    Geometry::Detector *detector = new Geometry::Detector("det",samplepos);
    Kernel::V3D pos;
    pos.spherical(r[i], angle[i], 0.0);
    detector->setPos(pos);

    // set detector ID, add copy to instrument and mark it
    detector->setID(detID[i]);
    instrument->add(detector);
    instrument->markAsDetector(detector);
  }

  // Now mark the up the monitors
  const int numMonitors = nxload.i_mon;     // The number of monitors
  const int* const monIndex = nxload.mdet;  // Index into the udet array for
  each monitor
  for (int j = 0; j < numMonitors; ++j)
  {
    const int detectorToMark = detID[monIndex[j]-1];
    Geometry::Detector *det =
  dynamic_cast<Geometry::Detector*>(instrument->getDetector(detectorToMark));
    det->markAsMonitor();
    g_log.information() << "Detector with ID " << detectorToMark << " marked as
  a monitor." << std::endl;
  }

  // Information to the user about what info is extracted from nexus file
  g_log.information() << "SamplePos component added with position set to
  (0,0,0).\n"
    << "Detector components added with position coordinates assumed to be
  relative to the position of the sample; \n"
    << "L2 and two-theta values were read from nexus file and used to set the r
  and theta spherical coordinates; \n"
    << "the remaining spherical coordinate phi was set to zero.\n"
    << "Source component added with position set to (0,-" << l1 << ",0). In
  standard configuration, with \n"
    << "the beam along y-axis pointing from source to sample, this implies the
  source is " << l1 << "m in front \n"
    << "of the sample. This value can be changed via the 'instrument.l1'
  configuration property.\n";
  */

  return;
}

} // namespace DataHandling
} // namespace Mantid
