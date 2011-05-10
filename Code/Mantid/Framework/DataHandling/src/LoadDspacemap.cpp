#include "MantidAPI/FileProperty.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataHandling/LoadCalFile.h"
#include "MantidDataHandling/LoadDspacemap.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidGeometry/IInstrument.h"
#include "MantidGeometry/V3D.h"
#include "MantidKernel/BinaryFile.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/System.h"
#include "MantidKernel/UnitFactory.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

namespace Mantid
{
namespace DataHandling
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(LoadDspacemap)
  
  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  LoadDspacemap::LoadDspacemap()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  LoadDspacemap::~LoadDspacemap()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void LoadDspacemap::initDocs()
  {
    this->setWikiSummary("Loads a Dspacemap file (POWGEN binary, VULCAN binary or ascii format) into an OffsetsWorkspace.");
    this->setOptionalMessage("Loads a Dspacemap file (POWGEN binary, VULCAN binary or ascii format) into an OffsetsWorkspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void LoadDspacemap::init()
  {
    // 3 properties for getting the right instrument
    LoadCalFile::getInstrument3WaysInit(this);

    std::vector< std::string > exts;
    exts.push_back(".dat");
    exts.push_back(".bin");

    declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
       "The DspacemapFile containing the d-space mapping.");

    std::vector<std::string> propOptions;
    propOptions.push_back("POWGEN");
    propOptions.push_back("VULCAN-ASCII");
    propOptions.push_back("VULCAN-Binary");
    declareProperty("FileType", "POWGEN", new ListValidator(propOptions),
      "The type of file being read.");

    declareProperty(new WorkspaceProperty<OffsetsWorkspace>("OutputWorkspace","",Direction::Output),
        "An output OffsetsWorkspace.");

  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void LoadDspacemap::exec()
  {
    // Get the instrument
    IInstrument_sptr inst = LoadCalFile::getInstrument3Ways(this);

    // Read in the calibration data
    const std::string DFileName = getProperty("Filename");

    // Create the blank output
    OffsetsWorkspace_sptr offsetsWS(new OffsetsWorkspace(inst));
    setProperty("OutputWorkspace", offsetsWS);

    std::string type = this->getPropertyValue("FileType");
    if (type == "POWGEN")
    {
      // generate map of the tof->d conversion factors
      CalculateOffsetsFromDSpacemapFile(DFileName, offsetsWS);
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
      this->CalculateOffsetsFromVulcanFactors(vulcan, offsetsWS);

    }

  }



  //-----------------------------------------------------------------------
  /**
   * Make a map of the conversion factors between tof and D-spacing
   * for all pixel IDs in a workspace.
   *
   * @param DFileName :: name of dspacemap file
   * @param offsetsWS :: OffsetsWorkspace to be filled.
   */
  void LoadDspacemap::CalculateOffsetsFromDSpacemapFile(const std::string DFileName,
      Mantid::DataObjects::OffsetsWorkspace_sptr offsetsWS)
  {
    // Get a pointer to the instrument contained in the workspace
    IInstrument_const_sptr instrument = offsetsWS->getInstrument();
    double l1;
    Geometry::V3D beamline,samplePos;
    double beamline_norm;
    instrument->getInstrumentParameters(l1,beamline,beamline_norm, samplePos);

    //To get all the detector ID's
    std::map<int, Geometry::IDetector_sptr> allDetectors;
    instrument->getDetectors(allDetectors);

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
      double factor = Instrument::calcConversion(l1, beamline, beamline_norm, samplePos, det, offset, false);
      offset=dspace[detectorID]/factor -1.0 ;
      // Save in the map
      try
      {
        offsetsWS->setValue(detectorID, offset);
      }
      catch (std::invalid_argument & e)
      {}
    }

  }






  //-----------------------------------------------------------------------
  /**
   * Make a map of the conversion factors between tof and D-spacing
   * for all pixel IDs in a workspace.
   *
   * @param vulcan :: map between detector ID and vulcan correction factor.
   * @param offsetsWS :: OffsetsWorkspace to be filled.
   */
  void LoadDspacemap::CalculateOffsetsFromVulcanFactors(std::map<int, double> & vulcan,
      Mantid::DataObjects::OffsetsWorkspace_sptr offsetsWS)
  {
    // Get a pointer to the instrument contained in the workspace
    IInstrument_const_sptr instrument = offsetsWS->getInstrument();

    //To get all the detector ID's
    std::map<int, Geometry::IDetector_sptr> allDetectors;
    instrument->getDetectors(allDetectors);

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
      vulcan_factor = pow(10.0,-vulcan_factor);

      // At this point, tof_corrected = vulcan_factor * tof_input
      // So this is the offset
      offset = vulcan_factor - 1.0;

      // Save in the map
      try
      {
        offsetsWS->setValue(detectorID, offset);
      }
      catch (std::invalid_argument & e)
      {}
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
  void LoadDspacemap::readVulcanAsciiFile(const std::string& fileName, std::map<int,double> & vulcan)
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
  void LoadDspacemap::readVulcanBinaryFile(const std::string& fileName, std::map<int,double> & vulcan)
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



} // namespace Mantid
} // namespace DataHandling

