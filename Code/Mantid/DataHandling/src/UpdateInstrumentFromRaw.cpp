//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/UpdateInstrumentFromRaw.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/Component.h"
#include "LoadRaw/isisraw.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/FileProperty.h"

#include <fstream>


namespace Mantid
{
namespace DataHandling
{

DECLARE_ALGORITHM(UpdateInstrumentFromRaw)

using namespace Kernel;
using namespace API;
using Geometry::Instrument;

/// Empty default constructor
UpdateInstrumentFromRaw::UpdateInstrumentFromRaw()
{}

/// Initialisation method.
void UpdateInstrumentFromRaw::init()
{
  // When used as a sub-algorithm the workspace name is not used - hence the "Anonymous" to satisfy the validator
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("Workspace","Anonymous",Direction::InOut),
    "The name of the workspace in which to store the imported instrument" );
  
  std::vector<std::string> exts;
  exts.push_back(".raw");
  exts.push_back(".s*");
  declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
		  "The filename (including its full or relative path) of an ISIS RAW file.\n"
		  "The file extension must either be .raw or .s??" ); 
}

/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 *
 *  @throw FileError Thrown if unable to parse XML file
 */
void UpdateInstrumentFromRaw::exec()
{
  // Retrieve the filename from the properties
  m_filename = getPropertyValue("Filename");

  // Get the input workspace
  const MatrixWorkspace_sptr localWorkspace = getProperty("Workspace");

  // open raw file
  ISISRAW iraw(NULL);
  if (iraw.readFromFile(m_filename.c_str(),false) != 0)
  {
    g_log.error("Unable to open file " + m_filename);
    throw Exception::FileError("Unable to open File:" , m_filename);
  }

  // Get reference to Instrument
  boost::shared_ptr<Instrument> instrument = localWorkspace->getBaseInstrument();
  if (instrument.get() == 0)
  {
      g_log.error("Trying to use ParInstrument as an Instrument.");
      throw std::runtime_error("Trying to use ParInstrument as an Instrument.");
  }

  // get instrument samplepos 

  Geometry::IObjComponent_sptr samplepos = instrument->getSample();

  progress(0.5);

  // add detectors
  const int numDetector = iraw.i_det;    // number of detectors
  const int* const detID = iraw.udet;    // detector IDs
  const float* const r = iraw.len2;      // distance from sample
  const float* const angle = iraw.tthe;  // angle between indicent beam and direction from sample to detector (two-theta)
  const float* const phi=iraw.ut;
  // Is ut01 (=phi) present? Sometimes an array is present but has wrong values e.g.all 1.0 or all 2.0
  bool phiPresent = iraw.i_use>0 && phi[0]!= 1.0 && phi[0] !=2.0; 

  double prog=0.5;
  for (int i = 0; i < numDetector; ++i)
  {
    // check if detID[i] is represented in IDF and if yes set position
    try
    {
      // throws exception if not found
      Geometry::IDetector_sptr det = instrument->getDetector(detID[i]);

      // get postion of parent
      Geometry::V3D parentPos = det->getPos() - det->getRelativePos();

      // Get position from raw file.
      Geometry::V3D pos;

      if(phiPresent)
        pos.spherical(r[i], angle[i], phi[i]);
      else
        pos.spherical(r[i], angle[i], 0.0 );

      // sets new relative position to parent so that its absolute position
      // is where the raw file says it is.
      det->setPos(pos-parentPos);
    }
    catch (...)
    {
      // if detector was not found do nothing in this case
    }
 
    progress(prog);
  }  

  return;
}

} // namespace DataHandling
} // namespace Mantid
