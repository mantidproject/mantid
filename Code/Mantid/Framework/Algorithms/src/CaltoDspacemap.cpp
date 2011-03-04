//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/AlignDetectors.h"
#include "MantidAlgorithms/CaltoDspacemap.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/V3D.h"
#include "MantidKernel/BinaryFile.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"
#include <cmath>
#include <fstream>

namespace Mantid
{
namespace Algorithms
{
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(CaltoDspacemap)

/// Sets documentation strings for this algorithm
void CaltoDspacemap::initDocs()
{
  this->setWikiSummary("Creates a Dspacemap file from calibration file with offsets calculated. ");
  this->setOptionalMessage("Creates a Dspacemap file from calibration file with offsets calculated. ");
}


using namespace Kernel;
using namespace API;
using Geometry::IInstrument_const_sptr;
using DataObjects::EventList;
using DataObjects::EventWorkspace;
using DataObjects::EventWorkspace_sptr;
using DataObjects::EventWorkspace_const_sptr;




//========================================================================
//========================================================================
/// (Empty) Constructor
CaltoDspacemap::CaltoDspacemap()
{
}

/// Destructor
CaltoDspacemap::~CaltoDspacemap()
{
}

//-----------------------------------------------------------------------
void CaltoDspacemap::init()
{
  declareProperty(
    new WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace","",Direction::Input),
    "A workspace with units of TOF" );

  declareProperty(new FileProperty("CalibrationFile", "", FileProperty::Load, ".cal"),
     "The CalFile on input contains the offsets");

  declareProperty(new FileProperty("DspacemapFile", "", FileProperty::Save, ".dat"),
     "The DspacemapFile on output contains the d-space mapping");

  declareProperty("PadDetID", 300000, "Pad Data to this number of pixels");

}





//-----------------------------------------------------------------------
/** Executes the algorithm
 *  @throw Exception::FileError If the calibration file cannot be opened and read successfully
 *  @throw Exception::InstrumentDefinitionError If unable to obtain the source-sample distance
 */
void CaltoDspacemap::exec()
{
  // Get the input workspace
  const MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

  // Read in the calibration data
  const std::string DFileName = getProperty("DspacemapFile");

  const std::string calFileName = getProperty("CalibrationFile");
  std::map<int,double> offsets;
  std::map<int,int> groups;
  progress(0.0,"Reading calibration file");
  if ( ! AlignDetectors::readCalFile(calFileName, offsets, groups) )
    throw Exception::FileError("Problem reading calibration file", calFileName);

  // generate map of the tof->d conversion factors
  CalculateDspaceFromCal(inputWS, DFileName, offsets);

}
//-----------------------------------------------------------------------
/**
 * Make a map of the conversion factors between tof and D-spacing
 * for all pixel IDs in a workspace.
 * @param inputWS the workspace containing the instrument geometry
 *    of interest.
 * @param DFileName name of dspacemap file
 * @param offsets map between pixelID and offset (from the calibration file)
 */
void CaltoDspacemap::CalculateDspaceFromCal(Mantid::API::MatrixWorkspace_const_sptr inputWS,
                                  std::string DFileName, 
                                  std::map<int,double> &offsets)
{
  const char * filename = DFileName.c_str();
  // Get a pointer to the instrument contained in the workspace
  IInstrument_const_sptr instrument = inputWS->getInstrument();
  double l1;
  Geometry::V3D beamline,samplePos;
  double beamline_norm;
  AlignDetectors::getInstrumentParameters(instrument,l1,beamline,beamline_norm, samplePos);

  //To get all the detector ID's
  const std::map<int, Geometry::IDetector_sptr> allDetectors = instrument->getDetectors();

  // Selects (empty, will default to true)
  std::map<int, bool> selects;

  std::map<int, Geometry::IDetector_sptr>::const_iterator it;
  int maxdetID = 0;
  for (it = allDetectors.begin(); it != allDetectors.end(); it++)
  {
    int detectorID = it->first;
    if(detectorID > maxdetID) maxdetID = detectorID;
  }
  int paddetID = getProperty("PadDetID");
  if (maxdetID < paddetID)maxdetID = paddetID;

  // Now write the POWGEN-style Dspace mapping file
  std::ofstream fout(filename, std::ios_base::out|std::ios_base::binary);
  Progress prog(this,0.0,1.0,maxdetID);

  for (int i = 0; i != maxdetID; i++)
  {
    //Compute the factor
    double factor;
    Geometry::IDetector_sptr det;
    for (it = allDetectors.begin(); it != allDetectors.end(); it++)
    {
      if(it->first == i) break;
    }
    det = it->second;
    if(det)
    {
      factor = AlignDetectors::calcConversion(l1, beamline, beamline_norm, samplePos, det, offsets[i], false);
      //Factor of 10 between ISAW and Mantid
      factor *= 0.1 ;
      fout.write( reinterpret_cast<char*>( &factor ), sizeof(double) );
    }
    else
    {
      factor = 0;
      fout.write( reinterpret_cast<char*>( &factor ), sizeof(double) );
    }
  }
  fout.close();
}

} // namespace Algorithms
} // namespace Mantid
