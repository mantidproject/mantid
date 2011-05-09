//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/UpdateInstrumentFromFile.h"
#include "MantidDataHandling/LoadRawHelper.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidAPI/FileProperty.h"
#include "LoadRaw/isisraw2.h"
#include <boost/scoped_ptr.hpp>

namespace Mantid
{
namespace DataHandling
{

DECLARE_ALGORITHM(UpdateInstrumentFromFile)

/// Sets documentation strings for this algorithm
void UpdateInstrumentFromFile::initDocs()
{
  this->setWikiSummary("Update detector positions initially loaded in from Instrument Definition File ([[InstrumentDefinitionFile|IDF]]) from information the given file. Note doing this will results in a slower performance (likely slightly slower performance) compared to specifying the correct detector positions in the IDF in the first place. It is assumed that the positions specified in the raw file are all with respect to the a coordinate system defined with its origin at the sample position.  Note that this algorithm moves the detectors without subsequent rotation, hence this means that detectors may not for example face the sample perfectly after this algorithm has been applied. ");
  this->setOptionalMessage("Updates detector positions initially loaded in from the Instrument Definition File (IDF) with information from the provided file. Currently supports RAW and ISIS NeXus.");
}


using namespace Kernel;
using namespace API;
using Geometry::Instrument_sptr;

/// Empty default constructor
UpdateInstrumentFromFile::UpdateInstrumentFromFile()
{}

/// Initialisation method.
void UpdateInstrumentFromFile::init()
{
  // When used as a sub-algorithm the workspace name is not used - hence the "Anonymous" to satisfy the validator
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("Workspace","Anonymous",Direction::InOut),
    "The name of the workspace in which to store the imported instrument");
  
  std::vector<std::string> exts;
  exts.push_back(".raw");
  exts.push_back(".s*");
  declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
		                               "The filename of the input file.\n"
                                   "Currently supports RAW and ISIS NeXus."
                 );
}

/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 *
 *  @throw FileError Thrown if unable to parse XML file
 */
void UpdateInstrumentFromFile::exec()
{
  // Retrieve the filename from the properties
  const std::string filename = getPropertyValue("Filename");
  MatrixWorkspace_const_sptr localWorkspace = getProperty("Workspace");
  Instrument_sptr instrument = localWorkspace->getBaseInstrument();
  if (instrument.get() == 0)
  {
    throw std::runtime_error("Input workspace has no defined instrument");
  }

  progress(0.5);
    
  // Check the file type
  boost::scoped_ptr<LoadRawHelper> rawCheck(new LoadRawHelper()); 
  if( rawCheck->fileCheck(filename) > 0 )
  {
    updateFromRaw(instrument, filename);
  }
  // Assume it is a NeXus file for now
  else
  {
    g_log.warning("Input file is not a RAW file.\n");
    //updateFromIsisNeXus(instrument, filename);
  }
}

/**
 * Update the detector information from a raw file
 * @param instrument :: A shared pointer to the base instrument, it will throw if a parametrized one is given
 * @param filename :: The input filename
 */
void UpdateInstrumentFromFile::updateFromRaw(Instrument_sptr instrument, const std::string & filename)
{
  ISISRAW2 iraw;
  if (iraw.readFromFile(filename.c_str(),false) != 0)
  {
    g_log.error("Unable to open file " + filename);
    throw Exception::FileError("Unable to open File:" , filename);
  }

  // add detectors
  const int numDetector = iraw.i_det;    // number of detectors
  const int* const detID = iraw.udet;    // detector IDs
  const float* const r = iraw.len2;      // distance from sample
  const float* const angle = iraw.tthe;  // angle between indicent beam and direction from sample to detector (two-theta)
  const float* const phi=iraw.ut;
  // Is ut01 (=phi) present? Sometimes an array is present but has wrong values e.g.all 1.0 or all 2.0
  bool phiPresent = iraw.i_use>0 && phi[0]!= 1.0 && phi[0] !=2.0; 

  g_log.information() << "Setting detector postions from RAW file.\n";
  double prog=0.5;
  for (int i = 0; i < numDetector; ++i)
  {
    try
    {
      // throws exception if not found
      Geometry::IDetector_sptr det = instrument->getDetector(detID[i]);
      // get postion of parent
      Geometry::V3D parentPos = det->getPos() - det->getRelativePos();
      // Get position from raw file.
      Geometry::V3D pos;
      if(phiPresent) pos.spherical(r[i], angle[i], phi[i]);
      else pos.spherical(r[i], angle[i], 0.0 );

      // sets new relative position to parent so that its absolute position
      // is where the raw file says it is.
      det->setPos(pos-parentPos);
    }
    catch (Kernel::Exception::NotFoundError&)
    {
      // if detector was not found do nothing in this case
    }
 
    progress(prog);
  }  
}

/**
 * Update the detector information from a raw file
 * @param instrument :: A shared pointer to the base instrument, it will throw if a parametrized one is given
 * @param filename :: The input filename
 */
void UpdateInstrumentFromFile::updateFromIsisNeXus(Instrument_sptr instrument, const std::string & filename)
{
  throw Kernel::Exception::NotImplementedError("UpdateInstrumentFromFile::updateFromIsisNeXus");
}

} // namespace DataHandling
} // namespace Mantid
