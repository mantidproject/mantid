//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/AlignDetectors.h"
#include "MantidAlgorithms/DspacemaptoCal.h"
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
DECLARE_ALGORITHM(DspacemaptoCal)

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
DspacemaptoCal::DspacemaptoCal()
{
}

/// Destructor
DspacemaptoCal::~DspacemaptoCal()
{
}

//-----------------------------------------------------------------------
void DspacemaptoCal::init()
{
  this->setOptionalMessage(
      "Write offset (.cal) file from dspacing file.\n"
      );

  declareProperty(
    new WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace","",Direction::Input),
    "A workspace with units of TOF" );

  std::vector< std::string > exts;
  exts.push_back(".dat");
  exts.push_back(".bin");

  declareProperty(new FileProperty("DspacemapFile", "", FileProperty::Load, exts),
     "The DspacemapFile containing the d-space mapping");

  std::vector<std::string> propOptions;
  propOptions.push_back("POWGEN");
  propOptions.push_back("VULCAN-ASCII");
  propOptions.push_back("VULCAN-Binary");
  declareProperty("FileType", "POWGEN", new ListValidator(propOptions),
    "The type of file being read.");

  declareProperty(new FileProperty("CalibrationFile", "", FileProperty::Load, ".cal"),
     "The CalFile on input contains the groups; on output contains the offsets");
}





//-----------------------------------------------------------------------
/** Executes the algorithm
 *  @throw Exception::FileError If the calibration file cannot be opened and read successfully
 *  @throw Exception::InstrumentDefinitionError If unable to obtain the source-sample distance
 */
void DspacemaptoCal::exec()
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

  std::string type = this->getPropertyValue("FileType");
  if (type == "POWGEN")
  {
    // generate map of the tof->d conversion factors
    CalculateOffsetsFromDSpacemapFile(inputWS, DFileName, calFileName, offsets, groups);
  }
  else
  {
    // Map of udet:funny vulcan correction factor.
    std::map<int,double> vulcan;
    if (type == "VULCAN-ASCII")
    {
      readVulcanAsciiFile(DFileName, vulcan);
    }
    else if (type == "VULCAN-Binary")
    {
      readVulcanBinaryFile(DFileName, vulcan);
    }
    else
      throw std::invalid_argument("Unexpected FileType property value received.");

    // Now that we have loaded the vulcan file (either type), convert it out.
    this->CalculateOffsetsFromVulcanFactors(inputWS, calFileName, vulcan, offsets, groups);

  }


}





//-----------------------------------------------------------------------
/**
 * Make a map of the conversion factors between tof and D-spacing
 * for all pixel IDs in a workspace.
 * @param inputWS the workspace containing the instrument geometry
 *    of interest.
 * @param DFileName name of dspacemap file
 * @param calFileName name of calibration/grouping file
 * @param offsets map between pixelID and offset (from the calibration file)
 * @param groups map between pixelID and group (from the calibration file)
 */
void DspacemaptoCal::CalculateOffsetsFromDSpacemapFile(Mantid::API::MatrixWorkspace_const_sptr inputWS,
                                  const std::string DFileName, std::string calFileName,
                                  std::map<int,double> &offsets, std::map<int,int> &groups)
{
  // Get a pointer to the instrument contained in the workspace
  IInstrument_const_sptr instrument = inputWS->getInstrument();
  double l1;
  Geometry::V3D beamline,samplePos;
  double beamline_norm;
  AlignDetectors::getInstrumentParameters(instrument,l1,beamline,beamline_norm, samplePos);

  //To get all the detector ID's
  const std::map<int, Geometry::IDetector_sptr> allDetectors = instrument->getDetectors();

  //Read in the POWGEN-style Dspace mapping file
  const char * filename = DFileName.c_str();
  std::ifstream fin(filename, std::ios_base::in|std::ios_base::binary);

  std::vector<double> dspace;
  double read;
  while (!fin.eof( )) 
  {
    fin.read( reinterpret_cast<char*>( &read ), sizeof read );
    //Factor of 10 between ISAW and Mantid
    read*=10.;
    dspace.push_back(read);
  }

  // Selects (empty, will default to true)
  std::map<int, bool> selects;

  std::map<int, Geometry::IDetector_sptr>::const_iterator it;
  for (it = allDetectors.begin(); it != allDetectors.end(); it++)
  {
    int detectorID = it->first;
    Geometry::IDetector_sptr det = it->second;

    //Compute the factor
    double offset = 0.0;
    double factor = AlignDetectors::calcConversion(l1, beamline, beamline_norm, samplePos, det, offset);
    offset=dspace[detectorID]/factor -1.0 ;
    // Save in the map
    offsets[detectorID] = offset;
  }

  // Now write it out
  this->WriteCalibrationFile(calFileName, allDetectors, offsets, selects, groups);
}






//-----------------------------------------------------------------------
/**
 * Make a map of the conversion factors between tof and D-spacing
 * for all pixel IDs in a workspace.
 * @param inputWS the workspace containing the instrument geometry
 *    of interest.
 * @param DFileName name of dspacemap file
 * @param calFileName name of calibration/grouping file
 * @param offsets map between pixelID and offset (from the calibration file)
 * @param groups map between pixelID and group (from the calibration file)
 */
void DspacemaptoCal::CalculateOffsetsFromVulcanFactors(Mantid::API::MatrixWorkspace_const_sptr inputWS,
                                  std::string calFileName, std::map<int, double> & vulcan,
                                  std::map<int,double> &offsets, std::map<int,int> &groups)
{
  // Get a pointer to the instrument contained in the workspace
  IInstrument_const_sptr instrument = inputWS->getInstrument();

  //To get all the detector ID's
  const std::map<int, Geometry::IDetector_sptr> allDetectors = instrument->getDetectors();

  // Selects (empty, will default to true)
  std::map<int, bool> selects;

  std::map<int, Geometry::IDetector_sptr>::const_iterator it;
  for (it = allDetectors.begin(); it != allDetectors.end(); it++)
  {
    int detectorID = it->first;
    Geometry::IDetector_sptr det = it->second;

    double offset = 0.0;

    // Find the vulcan factor;
    double vulcan_factor = 0.0;
    std::map<int,double>::const_iterator vulcan_iter = vulcan.find(detectorID);
    if( vulcan_iter != vulcan.end() )
      vulcan_factor = vulcan_iter->second;
    // The actual factor is 10^(-value_in_the_file)
    vulcan_factor = pow(10,-vulcan_factor);

    // At this point, tof_corrected = vulcan_factor * tof_input
    // So this is the offset
    offset = vulcan_factor - 1.0;

    // Save in the map
    offsets[detectorID] = offset;
  }

  // Now write it out
  this->WriteCalibrationFile(calFileName, allDetectors, offsets, selects, groups);
}



//-----------------------------------------------------------------------
/** Write out a .cal file
 *
 * @param calFileName :: output filename
 * @param allDetectors :: map of all the detectors (key = detector ID)
 * @param offsets :: map detectorID:offset to write; default 0.0 if not found
 * @param selects :: map detectorID:this_detector_is_selected; default true if not found
 * @param groups :: map detectorID:group #; default 1 if not found.
 */
void DspacemaptoCal::WriteCalibrationFile(std::string calFileName, const std::map<int, Geometry::IDetector_sptr> & allDetectors ,
                                  const std::map<int,double> &offsets, const std::map<int,bool> &selects, std::map<int,int> &groups)
{
  int number=0;

  // Header of the file
  std::ofstream fout(calFileName.c_str());
  fout <<"# Detector file with offsets.\n";
  fout <<"# Format: number    UDET         offset    select    group\n";

  std::map<int, Geometry::IDetector_sptr>::const_iterator it;
  for (it = allDetectors.begin(); it != allDetectors.end(); it++)
  {
    int detectorID = it->first;
    Geometry::IDetector_sptr det = it->second;

    //Find the offset, if any
    double offset = 0.0;
    std::map<int,double>::const_iterator off_iter = offsets.find(detectorID);
    if( off_iter != offsets.end() )
      offset = off_iter->second;

    //Find the group, if any
    int group = 1;
    std::map<int,int>::const_iterator grp_iter = groups.find(detectorID);
    if( grp_iter != groups.end() )
      group = grp_iter->second;

    // Find the selection, if any
    bool selected = true;
    std::map<int,bool>::const_iterator sel_iter = selects.find(detectorID);
    if( sel_iter != selects.end() )
      selected = sel_iter->second;

    if(group > 0)fout << std::fixed << std::setw(9) << number <<
        std::fixed << std::setw(15) << detectorID <<
        std::fixed << std::setprecision(7) << std::setw(15)<< offset <<
        std::fixed << std::setw(8) << (selected?1:0) <<
        std::fixed << std::setw(8) << group  << "\n";

     number++;
  }

}






//-----------------------------------------------------------------------
/** Reads an ASCII VULCAN filename where:
 *
 * 1st column : pixel ID
 * 2nd column : float "correction",  where corrected_TOF = initial_TOF / 10^correction
 *
 * @param fileName :: vulcan file name
 * @param[out] vulcan :: a map of pixel ID : correction factor in the file (2nd column)
 */
void DspacemaptoCal::readVulcanAsciiFile(const std::string& fileName, std::map<int,double> & vulcan)
{
  std::ifstream grFile(fileName.c_str());
  if (!grFile)
  {
      g_log.error() << "Unable to open vulcan file " << fileName << std::endl;
      return;
  }
  vulcan.clear();
  std::string str;
  while(getline(grFile,str))
  {
    if (str.empty() || str[0] == '#') continue;
    std::istringstream istr(str);
    int udet;
    double correction;
    istr >> udet >> correction;
    vulcan.insert(std::make_pair(udet,correction));
  }
}

/** Structure of the vulcan binary file */
struct VulcanCorrectionFactor
{
  /// ID for pixel
  double pixelID;
  /// Correction factor for pixel
  double factor;
};

//-----------------------------------------------------------------------
/** Reads a Binary VULCAN filename where:
 *
 * 1st 8 bytes : pixel ID
 * 2nd 8 bytes : double "correction",  where corrected_TOF = initial_TOF / 10^correction
 *
 * @param fileName :: vulcan file name
 * @param[out] vulcan :: a map of pixel ID : correction factor in the file (2nd column)
 */
void DspacemaptoCal::readVulcanBinaryFile(const std::string& fileName, std::map<int,double> & vulcan)
{
  BinaryFile<VulcanCorrectionFactor> file(fileName);
  std::vector<VulcanCorrectionFactor> * results = file.loadAll();
  if (results)
  {
    for (std::vector<VulcanCorrectionFactor>::iterator it = results->begin(); it!= results->end(); it++)
    {
      //std::cout << it->pixelID << " :! " << it->factor << std::endl;
      vulcan[it->pixelID] = it->factor;
    }
  }
}



} // namespace Algorithms
} // namespace Mantid
