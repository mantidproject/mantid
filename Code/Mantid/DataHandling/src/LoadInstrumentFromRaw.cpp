//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadInstrumentFromRaw.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/FileProperty.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/Component.h"
#include "LoadRaw/isisraw.h"
#include "MantidKernel/ArrayProperty.h"

#include <fstream>


namespace Mantid
{
namespace DataHandling
{

DECLARE_ALGORITHM(LoadInstrumentFromRaw)

using namespace Kernel;
using namespace API;
using Geometry::Instrument;

/// Empty default constructor
LoadInstrumentFromRaw::LoadInstrumentFromRaw()
{}

/// Initialisation method.
void LoadInstrumentFromRaw::init()
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
  declareProperty(new ArrayProperty<int>("MonitorList"),
      "List of detector ids of monitors loaded int to the workspace");
 
}

/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 *
 *  @throw FileError Thrown if unable to parse XML file
 */
void LoadInstrumentFromRaw::exec()
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

  // Clear off any existing instrument for this workspace
  localWorkspace->setInstrument(boost::shared_ptr<Instrument>(new Instrument));

  // Get reference to Instrument and set its name
  boost::shared_ptr<Geometry::Instrument> instrument = localWorkspace->getBaseInstrument();
  if (instrument.get() == 0)
  {
      g_log.error("Trying to use ParInstrument as an Instrument.");
      throw std::runtime_error("Trying to use ParInstrument as an Instrument.");
  }
  instrument->setName(iraw.i_inst);

  // Add dummy source and samplepos to instrument
  // The L2 and 2-theta values from Raw file assumed to be relative to sample position

  Geometry::ObjComponent *samplepos = new Geometry::ObjComponent("Sample",instrument.get());
  instrument->add(samplepos);
  instrument->markAsSamplePos(samplepos);
  samplepos->setPos(0.0,0.0,0.0);

  Geometry::ObjComponent *source = new Geometry::ObjComponent("Source",instrument.get());
  instrument->add(source);
  instrument->markAsSource(source);
  
  progress(0.5);
  double l1;
  // If user has provided an L1, use that
  if ( ! Kernel::ConfigService::Instance().getValue("instrument.L1", l1) )
  {
    // Otherwise try and get it from the raw file
    l1 = iraw.ivpb.i_l1;
    // Default to 10 if the raw file doesn't have it set
    if (l1 == 0)  l1 = 10.0;
  }
  source->setPos(0.0,0.0,-1.0*l1);

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
    // Create a new detector. Instrument will take ownership of pointer so no need to delete.
    Geometry::Detector *detector = new Geometry::Detector("det",samplepos);
    Geometry::V3D pos;

    if(phiPresent)
       pos.spherical(r[i], angle[i], phi[i]);
    else
       pos.spherical(r[i], angle[i], 0.0 );

    detector->setPos(pos);

    // set detector ID, add copy to instrument and mark it
    detector->setID(detID[i]);
    instrument->add(detector);
    instrument->markAsDetector(detector);
	prog+=(0.5/numDetector); 
	progress(prog);
  }  
   


  // Now mark the up the monitors
  const int numMonitors = iraw.i_mon;     // The number of monitors
  const int* const monIndex = iraw.mdet;  // Index into the udet array for each monitor
  
  for (int j = 0; j < numMonitors; ++j)
  {
    const int detectorToMark = detID[monIndex[j]-1];
    boost::shared_ptr<Geometry::IDetector> det = instrument->getDetector(detectorToMark);
	instrument->markAsMonitor(det.get());
    g_log.information() << "Detector with ID " << detectorToMark << " marked as a monitor." << std::endl;
  }
  std::vector<int> monitorList=instrument->getMonitors();
  setProperty("MonitorList",monitorList);
  // Information to the user about what info is extracted from raw file
  g_log.information() << "SamplePos component added with position set to (0,0,0).\n"
    << "Detector components added with position coordinates assumed to be relative to the position of the sample; \n"
    << "L2 and two-theta values were read from raw file and used to set the r and theta spherical coordinates; \n"
    << "the remaining spherical coordinate phi was set to zero.\n"
    << "Source component added with position set to (0,0,-" << l1 << "). In standard configuration, with \n"
    << "the beam along z-axis pointing from source to sample, this implies the source is " << l1 << "m in front \n"
    << "of the sample. This value can be changed via the 'instrument.l1' configuration property.\n";

  return;
}


} // namespace DataHandling
} // namespace Mantid
