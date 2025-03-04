// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidMuon/LoadInstrumentFromNexus.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidKernel/ConfigService.h"
#include "MantidMuon/MuonNexusReader.h"

#include <fstream>

namespace Mantid::Muon {

DECLARE_ALGORITHM(LoadInstrumentFromNexus)

using namespace Kernel;
using namespace API;
using Geometry::Instrument;

/// Empty default constructor
LoadInstrumentFromNexus::LoadInstrumentFromNexus() = default;

/// Initialisation method.
void LoadInstrumentFromNexus::init() {
  // When used as a Child Algorithm the workspace name is not used - hence the
  // "Anonymous" to satisfy the validator
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("Workspace", "Anonymous", Direction::InOut),
                  "The name of the workspace in which to attach the imported instrument");

  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, ".nxs"),
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
  Geometry::Instrument_sptr instrument(new Geometry::Instrument(nxload.getInstrumentName()));

  // Add dummy source and samplepos to instrument
  // The L2 and 2-theta values from nexus file assumed to be relative to sample
  // position

  Geometry::Component *samplepos = new Geometry::Component("Unknown", instrument.get());
  instrument->add(samplepos);
  instrument->markAsSamplePos(samplepos);
  samplepos->setPos(0.0, 0.0, 0.0);

  Geometry::ObjComponent *source = new Geometry::ObjComponent("Unknown", instrument.get());
  instrument->add(source);
  instrument->markAsSource(source);
  // If user has provided an L1, use that
  auto l1ConfigVal = Kernel::ConfigService::Instance().getValue<double>("instrument.L1");
  // Otherwise try and get it from the nexus file - but not there at present!
  // l1 = nxload.ivpb.i_l1;
  // Default to 10 if the file doesn't have it set
  double l1 = l1ConfigVal.value_or(10.0);

  source->setPos(0.0, -1.0 * l1, 0.0);
  localWorkspace->setInstrument(instrument);
  progress(1.0);
}

} // namespace Mantid::Muon
