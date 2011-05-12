#include "MantidDataHandling/SaveDspacemap.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FileProperty.h"
#include <fstream>


namespace Mantid
{
namespace DataHandling
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(SaveDspacemap)
  
  using namespace Mantid::Kernel;
  using namespace Mantid::API;
  using namespace Mantid::DataObjects;
  using namespace Mantid::Geometry;


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SaveDspacemap::SaveDspacemap()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  SaveDspacemap::~SaveDspacemap()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void SaveDspacemap::initDocs()
  {
    this->setWikiSummary("Saves an [[OffsetsWorkspace]] into a POWGEN-format binary dspace map file.");
    this->setOptionalMessage("Saves an OffsetsWorkspace into a POWGEN-format binary dspace map file.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void SaveDspacemap::init()
  {
    declareProperty(new WorkspaceProperty<OffsetsWorkspace>("InputWorkspace","",Direction::Input), "An input OffsetsWorkspace to save.");

    declareProperty(new FileProperty("DspacemapFile", "", FileProperty::Save, ".dat"),
       "The DspacemapFile on output contains the d-space mapping");

    declareProperty("PadDetID", 300000, "Pad Data to this number of pixels");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void SaveDspacemap::exec()
  {
    Mantid::DataObjects::OffsetsWorkspace_sptr offsetsWS = getProperty("InputWorkspace");
    std::string filename = getPropertyValue("DspacemapFile");
    CalculateDspaceFromCal(offsetsWS, filename);
  }


  //-----------------------------------------------------------------------
  /**
   * Make a map of the conversion factors between tof and D-spacing
   * for all pixel IDs in a workspace.
   *
   * @param DFileName name of dspacemap file
   * @param offsetsWS :: OffsetsWorkspace with instrument and offsets
   */
  void SaveDspacemap::CalculateDspaceFromCal(Mantid::DataObjects::OffsetsWorkspace_sptr offsetsWS,
                                    std::string DFileName)
  {
    const char * filename = DFileName.c_str();
    // Get a pointer to the instrument contained in the workspace
    IInstrument_const_sptr instrument = offsetsWS->getInstrument();
    double l1;
    Geometry::V3D beamline,samplePos;
    double beamline_norm;
    instrument->getInstrumentParameters(l1,beamline,beamline_norm, samplePos);

    //To get all the detector ID's
    std::map<detid_t, Geometry::IDetector_sptr> allDetectors;
    instrument->getDetectors(allDetectors);

    // Selects (empty, will default to true)
    std::map<int, bool> selects;

    std::map<detid_t, Geometry::IDetector_sptr>::const_iterator it;
    int64_t maxdetID = 0;
    for (it = allDetectors.begin(); it != allDetectors.end(); it++)
    {
      int64_t detectorID = it->first;
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
        factor = Instrument::calcConversion(l1, beamline, beamline_norm, samplePos, det, offsetsWS->getValue(i, 0.0), false);
        //Factor of 10 between ISAW and Mantid
        factor *= 0.1 ;
        if(factor<0)factor = 0.0;
        fout.write( reinterpret_cast<char*>( &factor ), sizeof(double) );
      }
      else
      {
        factor = 0;
        fout.write( reinterpret_cast<char*>( &factor ), sizeof(double) );
      }
      //Report progress
      prog.report();

    }
    fout.close();
  }


} // namespace Mantid
} // namespace DataHandling

