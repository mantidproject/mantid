//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/DspacemaptoCal.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidAPI/FileProperty.h"
#include "MantidGeometry/V3D.h"

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

const double CONSTANT = (PhysicalConstants::h * 1e10) / (2.0 * PhysicalConstants::NeutronMass * 1e6);

//-----------------------------------------------------------------------
/** Calculate the conversion factor for a single pixel.
 *
 * @param l1 :: Primary flight path.
 * @param beamline: vector = samplePos-sourcePos = a vector pointing from the source to the sample,
 *        the length of the distance between the two.
 * @param beamline_norm: (source to sample distance) * 2.0 (apparently)
 * @param samplePos: position of the sample
 * @param det: Geometry object representing the detector (position of the pixel)
 * @param offset: value (close to zero) that changes the factor := factor * (1+offset).
 */
double DcalcConversion(const double l1,
                      const Geometry::V3D &beamline,
                      const double beamline_norm,
                      const Geometry::V3D &samplePos,
                      const Geometry::IDetector_const_sptr &det,
                      const double offset)
{
  // Get the sample-detector distance for this detector (in metres)

  // The scattering angle for this detector (in radians).
  Geometry::V3D detPos = det->getPos();
  // Now detPos will be set with respect to samplePos
  detPos -= samplePos;
  // 0.5*cos(2theta)
  double l2=detPos.norm();
  double halfcosTwoTheta=detPos.scalar_prod(beamline)/(l2*beamline_norm);
  // This is sin(theta)
  double sinTheta=sqrt(0.5-halfcosTwoTheta);
  const double numerator = (1.0+offset);
  sinTheta *= (l1+l2);
  return (numerator * CONSTANT) / sinTheta;
}


//-----------------------------------------------------------------------
/**
 * Calculate the conversion factor (tof -> d-spacing)
 * for a LIST of detectors assigned to a single spectrum.
 */
double DcalcConversion(const double l1,
                      const Geometry::V3D &beamline,
                      const double beamline_norm,
                      const Geometry::V3D &samplePos,
                      const IInstrument_const_sptr &instrument,
                      const std::vector<int> &detectors,
                      const std::map<int,double> &offsets)
{
  double factor = 0.;
  double offset;
  for (std::vector<int>::const_iterator iter = detectors.begin(); iter != detectors.end(); ++iter)
  {
    std::map<int,double>::const_iterator off_iter = offsets.find(*iter);
    if( off_iter != offsets.end() )
    {
      offset = offsets.find(*iter)->second;
    }
    else
    {
      offset = 0.;
    }
    factor += DcalcConversion(l1, beamline, beamline_norm, samplePos,
                             instrument->getDetector(*iter), offset);
  }
  return factor / static_cast<double>(detectors.size());
}


//-----------------------------------------------------------------------
/**
 * Make a map of the conversion factors between tof and D-spacing
 * for all pixel IDs in a workspace.
 * @params: inputWS: the workspace containing the instrument geometry
 *    of interest.
 * @params: DFileName: name of dspacemap file
 * @params: calFileName: name of calibration/grouping file
 * @params: offsets: map between pixelID and offset (from the calibration file)
 * @params: groups: map between pixelID and group (from the calibration file)
 */
std::map<int, double> * DcalcTofToD_ConversionMap(Mantid::API::MatrixWorkspace_const_sptr inputWS,
                                  const std::string DFileName, std::string calFileName,
                                  const std::map<int,double> &offsets, std::map<int,int> &groups)
{
  std::map<int, double> * myMap = new std::map<int, double>();

  // Get a pointer to the instrument contained in the workspace
  IInstrument_const_sptr instrument = inputWS->getInstrument();

  // Get some positions
  const Geometry::IObjComponent_sptr sourceObj = instrument->getSource();
  if (sourceObj == NULL)
  {
	  throw Exception::InstrumentDefinitionError("Failed to get source component from instrument");
  }
  const Geometry::V3D sourcePos = sourceObj->getPos();
  const Geometry::V3D samplePos = instrument->getSample()->getPos();
  const Geometry::V3D beamline = samplePos-sourcePos;
  const double beamline_norm=2.0*beamline.norm();

  // Get the distance between the source and the sample (assume in metres)
  Geometry::IObjComponent_const_sptr sample = instrument->getSample();
  double l1;
  try
  {
    l1 = instrument->getSource()->getDistance(*sample);
  }
  catch (Exception::NotFoundError &e)
  {
    delete myMap;
    throw Exception::InstrumentDefinitionError("Unable to calculate source-sample distance", inputWS->getTitle());
  }

  //To get all the detector ID's
  std::map<int, Geometry::IDetector_sptr> allDetectors = instrument->getDetectors();

  //Now go through all
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
  std::map<int, Geometry::IDetector_sptr>::iterator it;
  int number=0;
  std::ofstream fout(calFileName.c_str());
  fout <<"# Detector file with offsets calculated from "<<  DFileName  <<"\n";
  fout <<"# Format: number    UDET         offset    select    group\n";
  for (it = allDetectors.begin(); it != allDetectors.end(); it++)
  {
    int detectorID = it->first;
    Geometry::IDetector_sptr det = it->second;

    //Find the offset, if any
    double offset;
    std::map<int,double>::const_iterator off_iter = offsets.find(detectorID);
    if( off_iter != offsets.end() )
      offset = off_iter->second;
    else
      offset = 0.;

    //Don't use offset from initial calibration file
    offset=0.;

    //Find the group, if any
    int group;
    std::map<int,int>::const_iterator grp_iter = groups.find(detectorID);
    if( grp_iter != groups.end() )
      group = grp_iter->second;
    else
      group = -1;

    //Compute the factor
    double factor = DcalcConversion(l1, beamline, beamline_norm, samplePos, det, offset);
    offset=dspace[detectorID]/factor -1.0 ;
    if(group > 0)fout << std::fixed << std::setw(9) << number <<
        std::fixed << std::setw(15) << detectorID <<
        std::fixed << std::setprecision(7) << std::setw(15)<< offset <<
        std::fixed << std::setw(8) << 1 <<
        std::fixed << std::setw(8) << group  << "\n";

     number++;

    //Save in map
    (*myMap)[detectorID] = factor;
  }

  //Give back the map.
  return myMap;
}


//========================================================================
//========================================================================
/// (Empty) Constructor
DspacemaptoCal::DspacemaptoCal()
{
  this->tofToDmap = NULL;
}

/// Destructor
DspacemaptoCal::~DspacemaptoCal()
{
  delete this->tofToDmap;
}

//-----------------------------------------------------------------------
void DspacemaptoCal::init()
{
  this->setOptionalMessage(
      "Write offset file from dspacing file.\n"
      );

  declareProperty(
    new WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace","",Direction::Input),
    "A workspace with units of TOF" );
  declareProperty(new FileProperty("DspacemapFile", "", FileProperty::Load, ".dat"),
     "The DspacemapFile containing the d-space mapping");
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
  if ( ! this->readCalFile(calFileName, offsets, groups) )
    throw Exception::FileError("Problem reading calibration file", calFileName);

  // generate map of the tof->d conversion factors
  this->tofToDmap = DcalcTofToD_ConversionMap(inputWS, DFileName, calFileName, offsets, groups);


}


//-----------------------------------------------------------------------
/// Reads the calibration file. Returns true for success, false otherwise.
bool DspacemaptoCal::readCalFile(const std::string& calFileName, std::map<int,double>& offsets, std::map<int,int>& groups)
{
    std::ifstream grFile(calFileName.c_str());
    if (!grFile)
    {
        g_log.error() << "Unable to open calibration file " << calFileName << std::endl;
        return false;
    }

    offsets.clear();
    groups.clear();
    std::string str;
    while(getline(grFile,str))
    {
      if (str.empty() || str[0] == '#') continue;
      std::istringstream istr(str);
      int n,udet,select,group;
      double offset;
      istr >> n >> udet >> offset >> select >> group;
      offsets.insert(std::make_pair(udet,offset));
      groups.insert(std::make_pair(udet,group));
    }
    return true;
}


} // namespace Algorithms
} // namespace Mantid
