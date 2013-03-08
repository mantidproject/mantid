/*WIKI* 

The detection time delay for each detector is subtracted from the time of flight bin boundaries in the spectrum associated with that detector. It is required that all the monitors have the same time delay and if this is non-zero the delay time is added to all TOF values. It is important that the units of the input workspace are TOF in microseconds, that [[GroupDetectors]] has not been run and this algorithm is only applied once to a workspace.

Values for the partial pressure of 3He and wall thickness are added into the parameter map for each detector in a form that can be read by [[DetectorEfficiencyCor]]. That is, the values are assumed to be in atmosheres and meters, respectively, and the properties are stored internally in the workspace parameter map as "3He(atm)" and "wallT(m)".  The values are likely to be read from the same RAW file that the workspace was loaded from or a DAT file that corrosponds to the same run or series of experimental runs.

Spectra whose associated detector data are not found in the input DAT or RAW file will not have their time of flight bin boundaries adjusted. Similarly nothing will be written to the parameter map for those detectors, the algorithm will continue to process data as normal but a warning will be written to the log. Detectors that are listed in the input file but do not exist in the instrument definition file will be ignored and details will be written to the log.

If all the time offsets are the same and the file ''appears'' to contain enough data for all detectors all detector spectra will use same bin boundaries, where possible. This will make the algorithm run much more quickly and use less memory.

When using a RAW file the time offset for each detector is read from the "hold off" table in the file's header while pressure and wall thickness data must be present in the user table array. The format for .DAT files is specified in the document "DETECTOR.DAT format" written by Prof G Toby Perring ("detector format.doc")

If the RelocateDets option is set to true, it is false by default, then the detectors are moved to the corresponding positions specified in the data file provided.

=== Detectors.dat data format ===

The the detector data can be stored as ASCII or [http://download.nexusformat.org/ NeXus] data file. The description of the ASCII DETECTOR.dat file is provided in the table below. Nexus file format can come in two flavours. The first one is completely equivalent to the ASCII 19 column data format and introduced to increase the speed of accessing these data in binary format, where the second one left for compatibility with Libisis. It has meaningful data corresponding to the columns 1-6, 17&18 below, but does not support multiple tube pressures and wall thickness. 

The [[LoadDetectorInfo]] algorithm currently reads and interprets rows 1-6,17&18 of the table, provided below (or columns of det.dat file) or correspondent data blocks from the NeXus file. It also does not understand the short (15 columns) MARI detector.dat ASCII file format (see '''ISIS detector.dat files''' below). 

==== Co-ordinate frames ====

For the purposes of the detector table we choose a right handed set of axes fixed in the spectrometer:

      x-axis  -- horizontal
      y-axis  -- vertical
      z-axis  -- parallel to ki

Centres of each detector element are defined in spherical polar co-ordinates as seen from the sample position:

  THETA Polar angle measured from the z-axis [what we would normally call the scattering angle PHI]. Note that  0< THETA <180
  PHI   Azimuthal angle measured from the x-axis in a right handed sense [what TGP, CDF	and RD01 call -BETA].
        For example, the West Bank of HET has PHI=0, the North Bank has PHI=90, the South Bank has PHI=270.

To specify the orientation of a detector, define axes x', y', z' as follows:

    x'-axis	increasing THETA
    y'-axis	increasing PHI
    z'-axis	parallel to the line joining sample and detector

The natural coordinate frame for the detector, xd, yd, zd, may not coincide with x', y', z'. For example, the natural frame for a gas tube is with zd along the axis of the tube, and the direction of xd chosen to be perpendicular to the line joining the detector with the sample. The characteristic dimensions of the detector, W_x, W_y, W_z, are given in the frame xd, yd, zd.

The detector coordinate axes xd, yd, zd are related to x', y', z' by a rotation. The transformation is be expressed by a three-component vector &alpha;<sub>x</sub>, &alpha;<sub>y</sub>, &alpha;<sub>z</sub>, where the magnitude of  the vector gives the angle of rotation in a right-hand sense, and the normalised elements give the components along x', y', z' of the unit vector about which the rotation takes place. The magnitude of the vector is in degrees. 

     e.g. non-PSD gas tube on the Debye-Scherrer cone:
       &alpha;<sub>x</sub> = -90<sup>0</sup>, &alpha;<sub>y</sub> = &alpha;<sub>z</sub> = 0<sup>0</sup>; Wx = Wy = 0.0254, Wz = 0.300

    e.g. Davidson bead monitor filling the HET beam at the monitor_2 position:
        &alpha;<sub>x</sub> = &alpha;<sub>y</sub> = &alpha;<sub>z</sub> = 0<sup>0</sup>; Wx = Wy = 0.045, Wz = 0.00025

Note that for PSD detectors the angles and dimensions refer to the pixel, not the whole tube. For HET, Wz = 0.914/64 = 0.01428.

==== File format ====
The file consists of number of ASCII columns, separated by spaces. All distances are in meters, and all angles in degrees.
{| border="1" cellpadding="5" cellspacing="0"
!Column Number
!Column Name
!Column Type
!Column Description
|-
| 1
|DET_NO
|integer
|Detector index number as in SPECTRA.DAT
|-
| 2
|DELTA
|real
|Electronics delay time (us). The origin is up to you. HOMER uses the peak in monitor_2 as the origin of time, so the only thing that really matters is the difference in the delay time between the detectors and the monitors.
|-
| 3
|L2
|real
|Sample - detector distance (m)
|-
| 4
|CODE
|integer
|Code number that describes the detector type.<span style="color:#FF0000"> Up to now this column has been redundant so the old files can contain unity for all detectors. </span>	Proper detectors should now follow the scheme:
   0	Dummy detector entry (see later)
   1	Davidson scintillator bead monitor (or just monitor)
   2	non-PSD gas tube
   3	PSD gas tube
|-
| 5
|THETA
|real
|Scattering angle (deg)
|-
| 6
|PHI
|real
|Azimuthal angle (deg). Origin and rotation sense defined above
|-
| 7
|W_x
|real
|True detector dimensions (m) in the frame xd’
|-
| 8
|W_y
|real
|True detector dimensions (m) in the frame yd’
|-
| 9
|W_z
|real
|True detector dimensions (m) in the frame zd’
|-
| 10
|F_x
|real
| False detector dimensions (m) in the frame xd' to avoid gaps between detectors
|-
| 11
|F_y
|real
|False detector dimensions (m) in the frame yd' to avoid gaps between detectors
|-
| 12
|F_z
|real
|False detector dimensions (m) in the frame zd' to avoid gaps between detectors
|-
| 13
|&alpha;<sub>x</sub>
|real
| x-coordinate of the vector describing orientation of detector in the co-ordinate frame defined above.
|-
| 14
|&alpha;<sub>y</sub>
|real
| y-coordinate of the vector describing orientation of detector in the co-ordinate frame defined above.
|-
| 15
|&alpha;<sub>z</sub>
|real
| z-coordinate of the vector describing orientation of detector in the co-ordinate frame defined above.
|-
|COLSPAN='4' |<span style="color:#0101DF">The columns with numbers higher then those described above contain information about the detectors that is dependent on the detector type: </span>
|-
|COLSPAN='4' |CODE = 0 (Dummy detector entry) :
|-
|-
| 16
| det_1
| real
|Frequently, some of the inputs to the data acquisition electronics do not have any detectors plugged into them. To ensure that any noise on these inputs is safely directed to a ‘dust-bin’ spectrum, they are given detector numbers which are associated with spectrum 0 in SPECTRA.DAT.  Dummy entries in DETECTOR.DAT are required for each of these dummy detectors. These entries should be given detector CODE = 0, which will be used to indicate that the other entries in DETECTOR.DAT can be ignored. For the sake of clarity, set all DELTA, L2...DET_4 to zero for dummy detectors.
|-
| 17
| det_2
| real
| The same as 16
|-
| 18
| det_3
| real
| The same as 16
|-
| 19
| det_4
| real
| The same as 16
|-
|-
|COLSPAN='4' |CODE = 1 (monitor) :
|-
| 16
| det_1
| real
| Dead time (us). Important for old detectors and high counting rate. 
|-
| 17
| det_2
| real
|Macroscopic absorption cross-section &Sigma;(m-1meV-0.5). For our monitors this is for Li scintillator glass. [I think I know what &Sigma; is approximately, but we don’t at present use it anywhere, so set to zero]
|-
| 18
| det_3
| real
| Ignored. Set to zero
|-
| 19
| det_4
| real
| Ignored. Set to zero
|-
|-
|COLSPAN='4' |CODE = 2 (non-PSD gas tube) :
|-
| 16
| det_1
| real
| Dead time (us). Important for old detectors and high counting rate. 
|-
| 17
| det_2
| real
|Gas tube detector 3He partial pressure (atms)
|-
| 18
| det_3
| real
| Gas tube wall thickness (m) [0.00080]
|-
| 19
| det_4
| real
| Ignored. Set to zero
|-
|-
|COLSPAN='4' |CODE = 3  (PSD gas tube) :
|-
| 16
| det_1
| real
| Dead time (us). Important for old detectors and high counting rate. 
|-
| 17
| det_2
| real
|Gas tube detector 3He partial pressure (atms) [10.0 or 6.4]
|-
| 18
| det_3
| real
| Gas tube wall thickness (m) [0.00080]
|-
| 19
| det_4
| real
|Index of tube to which the pixel belongs. Each PSD gas tube must be given a unique identifier. This enables programs that use DETECTOR.DAT to recognise that pixels have come from the same PSD tube.
|-

|}

==== ISIS DETECTOR.DAT files ====

The ISIS raw files seem to have two possible entries - MARI is non-standard for some reason

   nuse=14    nuse=10
   	 	(ncol=19) 	(ncol=15)
  det_no		spec		spec
  delta		delt		delt
  l2		len2		len2
  code		code		code
  theta		tthe		tthe
  phi		ut1		ut1
  W_x		ut2		ut2
  W_y		ut3		ut3
  W_z		ut4		ut4
  F_x		ut5
  F_y		ut6
  F_z		ut7
  &alpha;<sub>x</sub>		ut8		ut5
  &alpha;<sub>y</sub>		ut9		ut6
  &alpha;<sub>z</sub>		ut10		ut7
  det_1		ut11		
  det_2		ut12		ut8
  det_3		ut13		ut9
  det_4		ut14		ut10

*WIKI*/
#include "MantidDataHandling/LoadDetectorInfo.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidGeometry/Instrument/ComponentHelper.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include "LoadRaw/isisraw2.h"

#include <Poco/File.h>
#include <nexus/NeXusFile.hpp>

namespace Mantid
{
namespace DataHandling
{

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(LoadDetectorInfo)

/// Sets documentation strings for this algorithm
void LoadDetectorInfo::initDocs()
{
  this->setWikiSummary("Subtracts detector delay times from the time of flight X values in the workspace and modifies its information about detector pressures and wall thicknesses. This information can read from a DAT, RAW or NXS file that corresponds to the same run or series of experimental runs at given instrument configuration.");
  this->setOptionalMessage("Subtracts detector delay times from the time of flight X values in the workspace and modifies its information about detector pressures and wall thicknesses. This information can read from a DAT file or RAW file that corresponds to the same run or series of experimental runs as the workspace.");
}


using namespace Kernel;
using namespace API;
using namespace Geometry;

const float LoadDetectorInfo::UNSETOFFSET = float(-1e12);
const LoadDetectorInfo::detectDatForm LoadDetectorInfo::MARI_TYPE(10, 7, 8);
const LoadDetectorInfo::detectDatForm
  LoadDetectorInfo::MAPS_MER_TYPE(14, 11, 12);
/// Empty default constructor
LoadDetectorInfo::LoadDetectorInfo() 
  : Algorithm(), m_workspace(), m_numHists(-1), m_monitors(),
    m_monitorXs(), m_commonXs(false), m_monitOffset(UNSETOFFSET), m_error(false),
    m_FracCompl(0.0), m_samplePos(), m_pmap(NULL), m_instrument(), m_moveDets(false)
{
}

void LoadDetectorInfo::init()
{
  // Declare required input parameters for algorithm
  auto val = boost::make_shared<CompositeValidator>();
  val->add<WorkspaceUnitValidator>("TOF");
  val->add<HistogramValidator>();

  declareProperty(new WorkspaceProperty<>("Workspace","",Direction::InOut,val),
    "The name of the workspace to that the detector information will be loaded into" );
  std::vector<std::string> exts;
  // each of these allowed extensions must be dealt with in exec() below
  exts.push_back(".dat");
  exts.push_back(".raw");
  exts.push_back(".sca");
  exts.push_back(".nxs");
  declareProperty(new FileProperty("DataFilename","", FileProperty::Load, exts),
    "A .raw .DAT,nxs or sca file that contains information about the detectors in the "
    "workspace. The description of Dat and nxs file format is provided below.");
  
  declareProperty("RelocateDets", false,
      "If true then the detectors are moved to the positions specified in the raw/dat/nxs file.",
      Direction::Input);
}

/** Executes the algorithm
*  @throw invalid_argument if the detection delay time is different for different monitors
*  @throw FileError if there was a problem opening the file or its format
*  @throw MisMatch<int> if not very spectra is associated with exaltly one detector
*  @throw IndexError if there is a problem converting spectra indexes to spectra numbers, which would imply there is a problem with the workspace
*/
void LoadDetectorInfo::exec()
{
  // get the infomation that will be need from the user selected workspace, assume it exsists because of the validator in init()
  m_workspace = getProperty("Workspace");
  m_numHists = static_cast<int>(m_workspace->getNumberHistograms());
  // when we change the X-values will care take to maintain sharing. I have only implemented maintaining sharing where _all_ the arrays are initiall common
  m_commonXs = WorkspaceHelpers::sharedXData(m_workspace);
  // set the other member variables to their defaults
  m_FracCompl = 0.0;
  m_monitors.clear();
  m_monitOffset = UNSETOFFSET;
  m_error = false;
  m_moveDets = getProperty("RelocateDets");
  m_pmap = &(m_workspace->instrumentParameters());
  m_instrument = m_workspace->getInstrument();

  if( m_moveDets )
  {
    Geometry::IObjComponent_const_sptr sample = m_workspace->getInstrument()->getSample();
    if( sample ) m_samplePos = sample->getPos();
  }

  // get the user selected filename
  std::string filename = getPropertyValue("DataFilename");
  // load the data from the file using the correct algorithm depending on the assumed type of file
  if ( filename.find(".dat") == filename.size()-4 ||
    filename.find(".DAT") == filename.size()-4 )
  {
    readDAT(filename);
  }
  if ( filename.find(".sca") == filename.size()-4 ||
    filename.find(".SCA") == filename.size()-4 )
  {
    readDAT(filename);
  }
  
  if ( filename.find(".raw") == filename.size()-4 ||
    filename.find(".RAW") == filename.size()-4)
  {
    readRAW(filename);
  }

  if ( filename.find(".nxs") == filename.size()-4 ||
    filename.find(".NXS") == filename.size()-4)
  {
    readNXS(filename);
  }

  if (m_error)
  {
    g_log.warning() << "Note workspace " << getPropertyValue("Workspace") << " has been changed so if you intend to fix detector mismatch problems by running "
                    << name() << " on this workspace again is likely to corrupt it" << std::endl;
  }
  m_pmap = NULL; // Only valid for execution
  m_instrument.reset(); // Get rid of parameterized instrument reference promptly
}

/** Reads detector information from a .dat file, the file contains one line per detector
*  and its format is documented in "DETECTOR.DAT format" (more details in LoadDetectorInfo.h)
*  @param fName :: name and path of the data file to read
*  @throw invalid_argument if the detection delay time is different for different monitors
*  @throw FileError if there was a problem opening the file or its format
*  @throw MisMatch<int> if not very spectra is associated with exaltly one detector
*  @throw IndexError if there is a problem converting spectra indexes to spectra numbers, which would imply there is a problem with the workspace
*/
void LoadDetectorInfo::readDAT(const std::string& fName)
{
  g_log.information() << "Opening DAT file " << fName << std::endl;
  std::ifstream sFile(fName.c_str());
  if (!sFile)
  {
    throw Exception::FileError("Can't open file", fName);
  }
  // update the progress monitor and allow for user cancel
  progress(0.05);
  interruption_point();
 
  std::string str;
  // skip header line which contains something like <filename> generated by <prog>
  getline(sFile,str);
  g_log.debug() << "Reading " << fName << std::endl;
  g_log.information() << "Writing to the detector parameter map, only the first and last entries will be logged here\n";

  int detectorCount, numColumns;
  getline(sFile,str);
  std::istringstream header2(str);
  // header information is two numbers, the number of detectors, which we use but don't rely on, and the number of columns which we trust
  header2 >> detectorCount >> numColumns;
  if( detectorCount < 1 || numColumns != 14)
  {
    g_log.debug() << "Problem with the header information on the second line of the file, found: " << str << std::endl; 
    g_log.error() << name() << " requires that the input file has 14 columns and the number of detectors is positve\n";
    throw Exception::FileError("Incompatible file format found when reading line 2 in the input file", fName);
  }
 
  // skip title line
  getline(sFile,str);

  // will store all hte detector IDs that we get data for
  std::vector<detid_t> detectorList;
  detectorList.reserve(detectorCount);
  // stores the time offsets that the TOF X-values will be adjusted by at the end
  std::vector<float> offsets;
  offsets.reserve(detectorCount);
  float detectorOffset = UNSETOFFSET;
  bool differentOffsets = false;
  // used only for progress and logging
  std::vector<detid_t> missingDetectors;
  int count = 0, detectorProblemCount = 0;
  detectorInfo log;
  bool noneSet = true;
  // Now loop through lines, one for each detector or monitor. The latter are ignored.
  while( getline(sFile, str) )
  {
    if (str.empty() || str[0] == '#')
    {// comments and empty lines are allowed and ignored
      continue;
    }
    std::istringstream istr(str);
    
    detectorInfo readin;
    int code;
    float delta;
    // data for each detector in the file that we don't need
    float dump;

    // columns in the file, the detector ID and a code for the type of detector CODE = 3 (psd gas tube)
    istr >> readin.detID >> delta >> readin.l2 >> code >> readin.theta >> readin.phi;
    detectorList.push_back(readin.detID);
    offsets.push_back(delta);
    
    // check we have a supported code
    switch (code)
    {
      // these first two codes are detectors that we'll process, the code for this is below
      case PSD_GAS_TUBE : break;
      case NON_PSD_GAS_TUBE : break;

      // the following detectors codes specify little or no analysis
      case MONITOR_DEVICE :
        // throws invalid_argument if the detection delay time is different for different monitors
        noteMonitorOffset( delta, readin.detID);
        // skip the rest of this loop and move on to the next detector
        continue;

      // the detector is set to dummy, we won't report any error for this we'll just do nothing
      case DUMMY_DECT : continue;
      
      //we can't use data for detectors with other codes because we don't know the format, ignore the data and write to g_log.warning() once at the end
      default :
        detectorProblemCount ++;
        g_log.debug() << "Ignoring data for a detector with code " << code << std::endl;
        continue;
    }

    // gas filled detector specific code now until the end of this method
    
    // normally all the offsets are the same and things work faster, check for this
    if ( delta != detectorOffset )
    {// could mean different detectors have different offsets and we need to do things thoroughly
      if ( detectorOffset !=  UNSETOFFSET )
      {
        differentOffsets = true;
      }
      detectorOffset = delta;
    }

    //there are 10 uninteresting columns
    istr >> dump >> dump >> dump >> dump >> dump >> dump >> dump >> dump >> dump >> dump;
    // column names det_2   ,   det_3  , (assumes that code=3), the last column is not read
    istr >> readin.pressure >> readin.wallThick;
    try
    {
      setDetectorParams(readin, log);
      sometimesLogSuccess(log, noneSet);
    }
    catch (Exception::NotFoundError &)
    {// there are likely to be some detectors that we can't find in the instrument definition and we can't save parameters for these. We can't do anything about this just report the problem at the end
      missingDetectors.push_back(readin.detID);
      continue;
    }

    // report progress and check for a user cancel message at regualar intervals
    count ++;
    if ( count % INTERVAL == INTERVAL/2 )
    {
      progress(static_cast<double>(count)/(3*detectorCount));
      interruption_point();
    }
  }

  sometimesLogSuccess(log, noneSet = true);
  g_log.debug() << "Adjusting time of flight X-values by detector delay times, detectors have different offsets: "<< differentOffsets << std::endl;
  adjDelayTOFs(detectorOffset, differentOffsets, detectorList, offsets);
  
  if ( detectorProblemCount > 0 )
  {
    g_log.warning() << "Data for " << detectorProblemCount << " detectors that are neither monitors or psd gas tubes, the data have been ignored" << std::endl;
  }
  logErrorsFromRead(missingDetectors);
  g_log.debug() << "Successfully read DAT file " << fName << std::endl;
}

/** Reads data about the detectors from the header section of a RAW file it
* relies on the user table being in the correct format
* @param fName :: path to the RAW file to read
* @throw FileError if there is a problem opening the file or the header is in the wrong format
* @throw invalid_argument if the detection delay time is different for different monitors
* @throw MisMatch<int> if not very spectra is associated with exaltly one detector
* @throw IndexError if there is a problem converting spectra indexes to spectra numbers, which would imply there is a problem with the workspace
*/
void LoadDetectorInfo::readRAW(const std::string& fName)
{
  g_log.information() << "Opening RAW file " << fName << std::endl;
  // open raw file
  ISISRAW2 iraw;
  if (iraw.readFromFile(fName.c_str(),false) != 0)
  {
    g_log.error("Unable to open file " + fName);
    throw Exception::FileError("Unable to open File:" , fName);
  }
  // update the progress monitor and allow for user cancel
  progress(0.05);
  interruption_point();
  g_log.debug() << "Reading file " << fName << std::endl;
  g_log.information() << "Writing to the detector parameter map, only the first and last entries will be logged here\n";

  // the number of detectors according to the raw file header
  const int numDets = iraw.i_det;
  
  // there are different formats for where pressures and wall thinknesses are stored check the number of user tables
  detectDatForm tableForm;
  if ( iraw.i_use == static_cast<int>(MARI_TYPE.totalNumTabs) )
  {
    tableForm = MARI_TYPE;
  }
  else if ( iraw.i_use == static_cast<int>(MAPS_MER_TYPE.totalNumTabs) )
  {
    tableForm = MAPS_MER_TYPE;
  }
  else
  {
    g_log.warning() << "The user table has " << iraw.i_use << " entries expecting, " << name() << " expects " << MARI_TYPE.totalNumTabs << " or "  << MAPS_MER_TYPE.totalNumTabs << ". The workspace has not been altered\n";
    g_log.debug() << "This algorithm reads some data in from the user table. The data in the user table can vary between RAW files and we use the total number of user table entries and the code field as checks that we have the correct format\n";
    throw Exception::FileError("Detector gas pressure or wall thickness information is missing in the RAW file header or is in the wrong format", fName);
  }

  int detectorProblemCount = 0;
  std::vector<detid_t> missingDetectors;
  // the process will run a lot more quickly if all the detectors have the same offset time, monitors can have a different offset but it is an error if the offset for two monitors is different
  float detectorOffset =  UNSETOFFSET;
  bool differentOffsets = false;
  // there are only used to output to the log the first and last parameters that were stored
  detectorInfo log;
  bool noneSet = true;
  for (int i = 0; i < numDets; ++i)
  {
    // this code tells us what the numbers in the user table (iraw.ut), which we are about to use, mean
    switch (iraw.code[i])
    {
      // these first two codes are detectors that we'll process, the code for this is below
      case PSD_GAS_TUBE : break;
      case NON_PSD_GAS_TUBE : break;

      // the following detectors codes specify little or no analysis
      case MONITOR_DEVICE :
        // throws invalid_argument if the detection delay time is different for different monitors
        noteMonitorOffset(iraw.delt[i], iraw.udet[i]);
        // skip the rest of this loop and move on to the next detector
        continue;

      // the detector is set to dummy, we won't report any error for this we'll just do nothing
      case DUMMY_DECT : continue;
      
      //we can't use data for detectors with other codes because we don't know the format, ignore the data and write to g_log.warning() once at the end
      default :
        detectorProblemCount ++;
        g_log.debug() << "Ignoring detector with code " << iraw.code[i] << std::endl;
        continue;
    }

    // gas tube specific code now until the end of the for block

    // iraw.delt contains the all the detector offset times in the same order as the detector IDs in iraw.udet
    if ( iraw.delt[i] != detectorOffset )
    {// could mean different detectors have different offsets and we need to do things thoroughly
      if ( detectorOffset !=  UNSETOFFSET )
      {
        differentOffsets = true;
      }
      detectorOffset = iraw.delt[i];
    }

    detectorInfo readin;
    readin.detID = iraw.udet[i];
    readin.pressure = iraw.ut[i+tableForm.pressureTabNum*numDets];
    readin.wallThick = iraw.ut[i+tableForm.wallThickTabNum*numDets];

    // Get the detector info if we require it
    if( m_moveDets )
    {
      readin.l2 = iraw.len2[i];
      readin.theta = iraw.tthe[i];
      readin.phi = iraw.ut[i];
    }

    try
    {// iraw.udet contains the detector IDs and the other parameters are stored in order in the user table array (ut)
      setDetectorParams(readin, log);
      sometimesLogSuccess(log, noneSet);
    }
    catch (Exception::NotFoundError &)
    {// there are likely to be some detectors that we can't find in the instrument definition and we can't save parameters for these. We can't do anything about this just report the problem at the end
      missingDetectors.push_back(iraw.udet[i]);
      continue;
    }

    // report progress and check for a user cancel message sometimes
    if ( i % INTERVAL == INTERVAL/2 )
    {
      progress( static_cast<double>(i)/(3.*static_cast<double>(numDets)) );
      interruption_point();
    }
  }

  sometimesLogSuccess(log, noneSet = true);
  g_log.debug() << "Adjusting time of flight X-values by detector delay times" << std::endl;
  adjDelayTOFs(detectorOffset, differentOffsets, iraw.udet, iraw.delt,numDets);

  if ( detectorProblemCount > 0 )
  {
    g_log.warning() << detectorProblemCount << " entries in the user table had the wrong format, these data have been ignored and some detectors parameters were not updated" << std::endl;
  }
  logErrorsFromRead(missingDetectors);
  g_log.debug() << "Successfully read RAW file " << fName << std::endl;
}
/** Creates or modifies the parameter map for the specified detector adding
*  pressure and wall thickness information
*  @param params :: these will be written to the detector paraments 3He(atm)=pressure) and wallT(m)=wall thickness
*  @param change :: if the parameters are successfully changed they are stored here if doLogging below is set to true
*  @param doLogging:: if true, sets detectrorInfo &change to the current detector info value, if false, &change remains untouched by the routine
*  @throw NotFoundError if a pointer to the specified detector couldn't be retrieved
*/
void LoadDetectorInfo::setDetectorParams(const detectorInfo &params, detectorInfo &change,bool doLogging)
{
  Geometry::IDetector_const_sptr det = m_instrument->getDetector(params.detID);
  // Set the detectors pressure.
  m_pmap->addDouble(det->getComponentID(), "3He(atm)", params.pressure);
  // Set the wall thickness
  m_pmap->addDouble(det->getComponentID(), "wallT(m)", params.wallThick);

  // If we have a l2, theta and phi. Update the postion if required
  if( m_moveDets && 
      params.l2 != DBL_MAX && params.theta != DBL_MAX && params.phi != DBL_MAX )
  {
    V3D newPos;
    newPos.spherical(params.l2, params.theta, params.phi);
    // The sample position may not be at 0,0,0
    newPos += m_samplePos;

    ComponentHelper::moveComponent(*det, *m_pmap, newPos, ComponentHelper::Absolute);
  }


  // this operation has been successful if we are here, the following information is useful for logging
  if(doLogging)
    change = params;
}

/** Decides if the bin boundaries for all non-monitor spectra will be the same and runs the appropriate
*  function. It is possible for this function to set all detectors spectra X-values when they shouldn't
*  @param lastOffset :: the delay time of the last detector is only used if differentDelays is false or if detectID and delays are leave empty e.g. when we use common bins
*  @param differentDelays :: if the number of adjustments is greater than or equal to the number of spectra and this is false then common bins will be used
*  @param detectIDs :: the list of detector IDs needs to corrospond to the next argument, list of delays
*  @param delays :: ommitting or passing an empty list of delay times forces common bins to be used, any delays need to be in the same order as detectIDs
*/
void LoadDetectorInfo::adjDelayTOFs(double lastOffset, bool &differentDelays, const std::vector<detid_t> &detectIDs,
    const std::vector<float> &delays)
{
  // a spectra wont be adjusted if their detector wasn't included in the input file. So for differentDelays to false there need to be at least as many detectors in the data file as in the workspace
  differentDelays = differentDelays || ( static_cast<int>(delays.size()) < m_numHists );
  // if we don't have a list of delays then we have no choice
  differentDelays = (delays.size() > 0 && differentDelays);
  // see if adjusting the TOF Xbin boundaries requires knowledge of individual detectors or if they are all the same
  if ( differentDelays )
  {// not all the detectors have the same offset, do the offsetting thoroughly
    g_log.information() << "Adjusting all the X-values by their offset times, which varied depending on the detector. The offset time of the last detector is " << lastOffset << " microseconds" << std::endl;
    adjustXs(detectIDs, delays);
  }
  else
  {// all the detectors have the same offset _much_ easier to do
    g_log.information() << "Adjusting all the X-values by the constant offset time " << lastOffset << " microseconds" << std::endl;
    adjustXs(lastOffset);
  }
}
/** Decides if the bin boundaries for all non-monitor spectra will be the same and runs the appropiate
*  function. It is possible for this function to set all detectors spectra X-values when they shouldn't
*  @param lastOffset :: the delay time of the last detector is only used if differentDelays is false, e.g. we use common bins
*  @param differentDelays :: if the number of adjustments is greater than or equal to the number of spectra and this is false then common bins will be used
*  @param detectIDs :: the list of detector IDs is required regardless of how differentDelays is needs to be in the same order as delays
*  @param delays :: required regardless of if differentDelays is true or false and needs to be in the same order as detectIDs
*  @param numDetectors :: the size of the arrays pointed to by delays and detectIDs
*/void LoadDetectorInfo::adjDelayTOFs(double lastOffset, bool &differentDelays, const detid_t * const detectIDs,
    const float * const delays, int numDetectors)
{
  // a spectra wont be adjusted if their detector wasn't included in the RAW file. So for differentDelays to false there need to be at least as many detectors in the data file as in the workspace 
  differentDelays = differentDelays || ( numDetectors < m_numHists );
  if ( differentDelays )
  {
    const std::vector<detid_t> detectorList(detectIDs, detectIDs + numDetectors);
    const std::vector<float> offsets(delays, delays + numDetectors);
    adjDelayTOFs(lastOffset, differentDelays, detectorList, offsets);
  }
  else
  {
    adjDelayTOFs(lastOffset, differentDelays);
  }
}
/** This function finds the spectra associated with each detector passed ID and subtracts
*  the corrosponding value in the offsets array from all bin boundaries in that spectrum. If
*  all the X-values share the same storage array then some sharing is maintained
*  @param detIDs :: ID list of IDs numbers of all the detectors with offsets
*  @param offsets :: an array of values to change the bin boundaries by, these must be listed in the same order as detIDs
*  @throw MisMatch<int> if not every spectra is associated with exaltly one detector
*  @throw IndexError if there is a problem converting spectra indexes to spectra numbers, which would imply there is a problem with the workspace
*/
void LoadDetectorInfo::adjustXs(const std::vector<detid_t> &detIDs, const std::vector<float> &offsets)
{
  // getting spectra numbers from detector IDs is hard because the map works the other way, getting index numbers from spectra numbers has the same problem and we are about to do both
  // function adds zero if it can't find a detector into the new, probably large, multimap of detectorIDs to spectra numbers
  std::vector<specid_t> spectraList;
  m_workspace->getSpectraFromDetectorIDs(detIDs, spectraList);

  // allow spectra number to spectra index look ups
  spec2index_map specs2index;
  const SpectraAxis* axis = dynamic_cast<const SpectraAxis*>(m_workspace->getAxis(1));
  if (axis)
  {
    axis->getSpectraIndexMap(specs2index);
  }

  if ( spectraList.size() != detIDs.size() )
  {// this shouldn't really happen but would cause a crash if it weren't handled ...
    g_log.debug() << "Couldn't associate some detectors or monitors to spectra, are there some spectra missing?" << std::endl;
    throw Exception::MisMatch<size_t>(spectraList.size(), detIDs.size(), "Couldn't associate some detectors or monitors to spectra, are there some spectra missing?");
  }
  //used for logging
  std::vector<detid_t> missingDetectors;

  if ( m_commonXs )
  {// we can be memory efficient and only write a new set of bins when the offset has changed
    adjustXsCommon(offsets, spectraList, specs2index, missingDetectors);
  }//end shared X-values arrays
  else
  {// simplist case to code, adjust the bins in each spectrum
    adjustXsUnCommon(offsets, spectraList, specs2index, missingDetectors);
  }
  if ( !missingDetectors.empty() )
  {
    g_log.warning() << "The following detector IDs were read in the input file but aren't associated with any spectra: " << detIDs[missingDetectors[0]] << std::endl;
    for ( std::vector<int>::size_type i = 0; i < missingDetectors.size(); ++i )
    {
      g_log.warning() << ", " << detIDs[missingDetectors[i]] << std::endl;
    }
    g_log.warning() << "Data listed for those detectors was ignored" << std::endl;
  }
}
/** Substracts the given off value from all the bin boundaries in all the spectra. If the arrays
*  containing the X-values are all shared then they remain shared 
*  @param detectorOffset :: the value to remove from the bin boundaries
*  @throw  IndexError if there is a problem converting spectra indexes to spectra numbers, which would imply there is a problem with the workspace 
*/
void LoadDetectorInfo::adjustXs(const double detectorOffset)
{
  MantidVecPtr monitorXs;
  // for g_log keep a count of the number of spectra that we can't find detectors for in the raw file
  int spuriousSpectra = 0;
  double fracCompl = 1.0/3.0;

  MantidVecPtr newXs;

  for ( int specInd = 0; specInd < m_numHists; ++specInd )
  {
    // check if we dealing with a monitor as these are dealt by a different function
    const std::set<detid_t> & dets = m_workspace->getSpectrum(size_t(specInd))->getDetectorIDs();
    if ( !dets.empty() )
    {// is it in the monitors list
      if ( m_monitors.find(*dets.begin()) == m_monitors.end() )
      {// it's not a monitor, it's a regular detector 
        if ( (*newXs).empty() )
        {// we don't have any cached values from doing the offsetting previously, do the calculation
          if ( m_commonXs )
          {// common Xs means we only need to go through and change the bin boundaries once, we then copy this data
            // this must be first non-monitor spectrum has been found, this will be used as the base for all others
            setUpXArray(newXs, specInd, detectorOffset);
          }
          else// no common bins
          {// move the bin boundaries each time for each array
            MantidVec &Xbins = m_workspace->dataX(specInd);
            std::transform(Xbins.begin(), Xbins.end(), Xbins.begin(),
              std::bind2nd(std::minus<double>(), detectorOffset) );
          }
        }
        else// we have cached values in newXs
        {// this copies a pointer so that the histogram looks for it data in the correct cow_ptr
          m_workspace->setX(specInd, newXs);
        }
      }
      else// ( m_monitors.find(dets[0]) != m_monitors.end() )
      {// it's a monitor 
        if ( (*monitorXs).empty() )
        {// we have no cached values
          // negative because we add the monitor offset, not take away as for detectors
          setUpXArray(monitorXs, specInd, -m_monitOffset);
        }// this algorthim requires that all monitors have the same offset and so we can always use cached values
        else m_workspace->setX(specInd, monitorXs);
      }
    }
    else
    { //  The detector is not in the instrument definition file
      // we don't have any information on the spectrum and so we can't correct it
      for ( MantidVec::size_type j = 0; j < m_workspace->dataY(specInd).size(); j++ )
      {//as this algorithm is about obtaining corrected data I'll mark this this uncorrectable data as bad by setting it to zero
        m_workspace->dataY(specInd)[j] = 0;
        m_workspace->dataE(specInd)[j] = 0;
      }
      // as this stuff happens a lot don't write much to high log levels but do a full log to debug
      spuriousSpectra ++;
      if (spuriousSpectra == 1) g_log.debug() << "Missing detector information cause the following spectra to be set to zero, suspect missing detectors in instrument definition : " << specInd ;
      else g_log.debug() << "," << specInd;
    }
    if ( specInd % INTERVAL == INTERVAL/2 )
    {
      fracCompl += (2.0*static_cast<double>(INTERVAL)/3.0)/m_numHists;
      progress( fracCompl );
      interruption_point();
    }
  }//move on to the next histogram

  if (spuriousSpectra > 0)
  {
    g_log.debug() << std::endl;
    g_log.information() << "Found " << spuriousSpectra << " spectra without associated detectors, probably the detectors are not present in the instrument definition and this is not serious. The Y and error values for those spectra have be set to zero" << std::endl;
  }
}
/** A memory efficient function that adjusts the X-value bin boundaries that only creates a new
*  cow_ptr array when the offset has changed
* @param offsets :: an array of times to adjust all the bins in each workspace histogram by
* @param spectraList :: a list of spectra numbers in the same order as the offsets
* @param specs2index :: a map that allows finding a spectra indexes from spectra numbers
* @param missingDetectors :: this will be filled with the array indices of the detector offsets that we can't find spectra indices for
*/
void LoadDetectorInfo::adjustXsCommon(const std::vector<float> &offsets, const std::vector<specid_t> &spectraList,
    spec2index_map &specs2index, std::vector<detid_t> missingDetectors)
{
  // space for cached values
  float cachedOffSet = UNSETOFFSET;
  MantidVecPtr monitorXs;
  MantidVecPtr cachedXs;

  double fracCompl = 1.0/3.0;
  
  for ( std::vector<int>::size_type j = 0; j < spectraList.size(); ++j )
  {// first check that our spectranumber to spectra index map is working for us
    if ( specs2index.find(spectraList[j]) == specs2index.end() )
    {// we can't find the spectrum associated the detector prepare to log that
      missingDetectors.push_back(static_cast<int>(j));
      // and then move on to the next detector in the loop
      continue;
    }

    const size_t specIndex = specs2index[spectraList[j]];
    // check if we dealing with a monitor as these are dealt by a different function
    const std::set<detid_t> & dets = m_workspace->getSpectrum(specIndex)->getDetectorIDs();

    if ( !dets.empty() )
    {// is it in the monitors list
      if ( m_monitors.find(*dets.begin()) == m_monitors.end() )
      {// it's not a monitor, it's a regular detector
        if ( offsets[j] != cachedOffSet )
        {
          setUpXArray(cachedXs, specIndex, offsets[j]);
          cachedOffSet = offsets[j];
        }
        else m_workspace->setX(specIndex, cachedXs);
      }
      else
      {// it's a monitor 
        if ( (*monitorXs).empty() )
        {
          // negative because we add the monitor offset, not take away as for detectors, the difference between the monitor delay and the detectors that counts
          setUpXArray(monitorXs, specIndex, -m_monitOffset);
        }
        else m_workspace->setX(specIndex, monitorXs);
      }
    }
    if ( j % INTERVAL == INTERVAL/2 )
    {
      fracCompl += (2.0*INTERVAL/3.0)/static_cast<double>(spectraList.size());
      progress( fracCompl );
      interruption_point();
    }
  }
}
/** Adjusts the X-value bin boundaries given offsets and makes no assumptions about
* that the histograms have shared bins or the time offsets are the same
* @param offsets :: an array of times to adjust all the bins in each workspace histogram by
* @param spectraList :: a list of spectra numbers in the same order as the offsets
* @param specs2index :: a map that allows finding a spectra indexes from spectra numbers
* @param missingDetectors :: this will be filled with the array indices of the detector offsets that we can't find spectra indices for
*/
void LoadDetectorInfo::adjustXsUnCommon(const std::vector<float> &offsets, const std::vector<specid_t> &spectraList,
    spec2index_map &specs2index, std::vector<detid_t> missingDetectors)
{// the monitors can't have diferent offsets so I can cache the bin boundaries for all the monitors
  MantidVecPtr monitorXs;

  double fracCompl = 1.0/3.0;

  for ( std::vector<int>::size_type j = 0; j < spectraList.size(); ++j )
  {// first check that our spectranumber to spectra index map is working for us
    if ( specs2index.find(spectraList[j]) == specs2index.end() )
    {// we can't find the spectrum associated the detector prepare to log that
      missingDetectors.push_back(static_cast<int>(j));
      // and then move on to the next detector in the loop
      continue;
    }
    const size_t specIndex = specs2index[spectraList[j]];

    // check if we dealing with a monitor as these are dealt by a different function
    const std::set<detid_t> & dets = m_workspace->getSpectrum(specIndex)->getDetectorIDs();

    if ( !dets.empty() )
    {// is it in the monitors list
      if ( m_monitors.find(*dets.begin()) == m_monitors.end() )
      {// it's not a monitor, it's a regular detector
        MantidVec &Xbins = m_workspace->dataX(specIndex);
        std::transform(Xbins.begin(), Xbins.end(), Xbins.begin(),
          std::bind2nd(std::minus<double>(), offsets[j]) );
      }
      else
      {// it's a monitor 
        if ( (*monitorXs).empty() )
        {
          // negative because we add the monitor offset, not take away as for detectors, the difference between the monitor delay and the detectors is the quanity we are after
          setUpXArray(monitorXs, specIndex, -m_monitOffset);
        }
        else m_workspace->setX(specIndex, monitorXs);
      }
    }
    if ( j % INTERVAL == INTERVAL/2 )
    {
      fracCompl += (2.0*INTERVAL/3.0)/static_cast<double>(spectraList.size());
      progress( fracCompl );
      interruption_point();
    }
  }
}
/**Changes the TOF (X values) by the offset time monitors but it chacks that first that
*  the monitor offset is non-zero. Throws if not all MonitorOffsets are the same
*  @param offSet :: the next offset time for a detector that was read in
*  @param detID :: the the monitor's detector ID number
*  @throw invalid_argument if it finds a monitor that has a different offset from the rest
*/
void LoadDetectorInfo::noteMonitorOffset(const float offSet, const detid_t detID)
{
  // this algorithm assumes monitors have the same offset (it saves looking for the "master" or "time zero" monitor). So the first time this function is called we accept any offset, on subsequent calls we check
  if ( offSet != m_monitOffset && m_monitOffset != UNSETOFFSET )
  {// this isn't the first monitor we've found so we can check it has the same offset as the previous ones
    g_log.error() << "Found one monitor with an offset time of " << m_monitOffset << " and another with an offset of " << offSet << std::endl;
    throw std::invalid_argument("All monitors must have the same offset");
  }
  m_monitors.insert(detID);
  // this line will only change the value of m_monitOffset the first time, after that it's redundant
  m_monitOffset = offSet;
}

/** Modify X-values from the workspace and store them in the shared array
* containted within the cow pointer
* @param theXValuesArray :: this will contain the new values in it's array
* @param specInd :: index number of histogram from with to take the original X-values 
* @param offset :: _subtract_ this number from all the X-values
*/
void LoadDetectorInfo::setUpXArray(MantidVecPtr &theXValuesArray, size_t specInd, double offset)
{
  std::vector<double> &AllXbins = theXValuesArray.access();
  AllXbins.resize(m_workspace->dataX(specInd).size());
  std::transform(
    m_workspace->readX(specInd).begin(), m_workspace->readX(specInd).end(),
    AllXbins.begin(),
    std::bind2nd(std::minus<double>(), offset) );
  m_workspace->setX(specInd, theXValuesArray);
}
/** Reports information on detectors that we couldn't get a pointer to, if any of these errors occured
*  @param missingDetectors :: detector IDs of detectors that we couldn't get a pointer to
*/
void LoadDetectorInfo::logErrorsFromRead(const std::vector<detid_t> &missingDetectors)
{
  if ( missingDetectors.size() > 0 )
  {
    g_log.warning() << "Entries exist in the input file for " << missingDetectors.size() << " detectors that could not be accessed, data ignored. Probably the detectors are not present in the instrument definition\n";
    g_log.information() << "The detector IDs are: " << missingDetectors[0];
    for ( std::vector<int>::size_type k = 1; k < missingDetectors.size(); k++ )
    {
      g_log.information() << ", " << missingDetectors[k];
    }
    g_log.information() << std::endl;
    m_error = true;
  }
}
/** write the parameters that were passed to the log. To make it easier to reduce the amount
*  of logging this function will set the last parameter to false
*  @param params :: constains the data to log
*  @param needToLog :: only log if this is set to true, the variable is then set to false
*/
void LoadDetectorInfo::sometimesLogSuccess(const detectorInfo &params, bool &needToLog)
{
  if (needToLog)
  {
    g_log.information() << name() << " has set pressure=" << params.pressure << " and wall thickness=" << params.wallThick << " for the detector with ID " << params.detID << std::endl;
  }
  needToLog = false;
}

/**The methor reads selected part of detector.nxs file and apply correspondent changes to the detectors */ 
void LoadDetectorInfo::readNXS(const std::string& fName)
{
  auto hFile = new ::NeXus::File(fName,NXACC_READ);

  std::map<std::string, std::string> entries;
  hFile->getEntries(entries);
  // define data requested
  bool detDataFound(false);
  std::vector<detectorInfo> detStruct;
  std::vector<int32_t> detType;
  std::vector<float> detOffset;
  std::vector<detid_t> detectorList;
  // identify what file we would read
  if(entries.find("full_reference_detector")!=entries.end())
  {
    g_log.warning()<<" reading data from old Libisis format, which does not support multiple helium pressures and wall thickness\n";
    hFile->openGroup("full_reference_detector","NXIXTdetector");
    this->readLibisisNXS(hFile,detStruct,detType,detOffset,detectorList);
    hFile->closeGroup();
    detDataFound = true;
  }
  if(entries.find("detectors.dat")!=entries.end())
  {  
    hFile->openGroup("detectors.dat","NXEntry");
    this->readDetDotDatNXS(hFile,detStruct,detType,detOffset,detectorList);
    hFile->closeGroup();
    detDataFound = true;
  }
  delete hFile;

  if(!detDataFound)
    throw std::invalid_argument("the NeXus file "+fName+" does not contain necessary detector's information");

  g_log.notice() << "Detectors indo loaded from nexus file, starting applying corrections\n";
  // adjust progress and allow user to cancel
  progress(0.1);  
  interruption_point();



  // process detectors and modify instrument
  size_t nDetectors = detStruct.size();
  float detectorOffset = UNSETOFFSET;
  bool differentOffsets = false; 
  std::vector<detid_t> missingDetectors;
  detectorInfo log;

  bool noneSet = true;
  size_t detectorProblemCount(0);
  //PRAGMA(omp parallel for reduction(|| : differentOffsets)) 
  for(int i=0;i<int(nDetectors);i++)
  {
    //PARALLEL_START_INTERUPT_REGION

    // check we have a supported code
    switch (detType[i])
    {
    // these first two codes are detectors that we'll process, the code for this is below
    case PSD_GAS_TUBE : break;
    case NON_PSD_GAS_TUBE : break;

    // the following detectors codes specify little or no analysis
    case MONITOR_DEVICE :
      //PARALLEL_CRITICAL(different_mon_offset)
      {
        // throws invalid_argument if the detection delay time is different for different monitors
        noteMonitorOffset(detOffset[i], detStruct[i].detID);
      }
      // skip the rest of this loop and move on to the next detector
      continue;

      // the detector is set to dummy, we won't report any error for this we'll just do nothing
    case DUMMY_DECT : continue;

    //we can't use data for detectors with other codes because we don't know the format, ignore the data and write to g_log.warning() once at the end
    default :
      //PARALLEL_CRITICAL(problem_detector)
      {
        detectorProblemCount ++;
        g_log.debug() << "Ignoring data for a detector with code " << detType[i] << std::endl;
      }
      continue;
    }

    // gas filled detector specific code now until the end of this method

    // normally all the offsets are the same and things work faster, check for this
    if ( detOffset[i] != detectorOffset )
    {// could mean different detectors have different offsets and we need to do things thoroughly
      if ( detectorOffset ==  UNSETOFFSET ) 
      {
        //PARALLEL_CRITICAL(det_offset)
        {
          if(detectorOffset ==  UNSETOFFSET) detectorOffset = detOffset[i];
          if(detOffset[i] != detectorOffset) differentOffsets=true;
        }
      }
      else
      {
        differentOffsets=true;
      }
    }


    bool exception(false);
    try
    {
        setDetectorParams(detStruct[i], log);
        sometimesLogSuccess(log, noneSet);
    }
    catch (Exception::NotFoundError &)
    {// there are likely to be some detectors that we can't find in the instrument definition and we can't save parameters for these. We can't do anything about this just report the problem at the end
        //PARALLEL_CRITICAL(non_existing_detector)
      {
          missingDetectors.push_back(detStruct[i].detID);
      }
        // Set the flag to signal that we should call continue outside of the catch block. Works around a defect with the Intel compiler.
       exception = true;
    }
    if ( exception ) continue;   

    // report progress and check for a user cancel message at regualar intervals
    if ( i % 100 == 0 )
    {	
        //PARALLEL_CRITICAL(logging)
        {
            //log = detStruct[i];
            //sometimesLogSuccess(log, noneSet);
            progress(static_cast<double>(i));
            interruption_point();
        }
    }
        
    //PARALLEL_END_INTERUPT_REGION
  }
  //PARALLEL_CHECK_INTERUPT_REGION

  sometimesLogSuccess(log, noneSet = true);
  g_log.notice() << "Adjusting time of flight X-values by detector delay times, detector have the different offsets:  "<< differentOffsets << std::endl;
  adjDelayTOFs(detectorOffset, differentOffsets, detectorList, detOffset);
  
    if ( detectorProblemCount > 0 )
    {
      g_log.warning() << "Data for " << detectorProblemCount << " detectors that are neither monitors or psd gas tubes, the data have been ignored" << std::endl;
    }
    logErrorsFromRead(missingDetectors);
    g_log.debug() << "Successfully read DAT file " << fName << std::endl;
}
/**Read detector.dat information from the old libisis NeXus format 
 * @param hFile -- pointer to the opened NeXus file handle, opened at the group, which contains Libisis Detector information
 * @param  detStruct -- the vector of the DetectorInfo structures, describing the detectors to modify
 * @param  detType   -- the vector of the detector types, described in the algorithm description
 * @param detOffset  -- the time delay for each detector's electronics, which should be corrected for.
 * @param detList  -- list of detectors
*/
void LoadDetectorInfo::readLibisisNXS(::NeXus::File *hFile, std::vector<detectorInfo> &detStruct, std::vector<int32_t>&detType,std::vector<float> &detOffset,
                                      std::vector<detid_t>&detList)
{

    std::vector<double> delayTime;
    std::vector<int32_t> detID;
    // read detector ID
    hFile->readData<int32_t>("det_no",detID);
    // read detector type 
    hFile->readData<int32_t>("det_type",detType);
    // read the detector's type
    hFile->readData<double>("delay_time",delayTime);

    size_t nDetectors = delayTime.size();
    std::vector<double> L2,Phi,Theta;
    if(m_moveDets)
    {
      // the secondary flight path -- sample to detector
        hFile->readData<double>("L2",L2);
      // detector's polar angle Theta (2Theta in Brag's terminology)
        hFile->readData<double>("theta",Theta);
      // detector's polar angle, phi
        hFile->readData<double>("phi",Phi);
    }
    else
    {
      L2.assign(nDetectors,DBL_MAX);
      Phi.assign(nDetectors,DBL_MAX);
      Theta.assign(nDetectors,DBL_MAX);
    }
    //We need He3 pressue and wall thikness
    double pressure(0.0008),wallThickness(10.);
    hFile->openGroup("det_he3","NXIXTdet_he3");
        hFile->readData<double>("gas_pressure",pressure);
        hFile->readData<double>("wall_thickness",wallThickness);
    hFile->closeGroup();
    if(pressure<=0)
    {
      g_log.warning("The data file does not contain correct He3 pressure, default value of 10Bar is used instead");
      pressure = 10.;
    }
    if(wallThickness<=0)
    {
      g_log.warning("The data file does not contain correct detector's wall thickness, default value of 0.8mm is used instead");
      wallThickness = 0.0008;
    }


   if(nDetectors!=L2.size()||nDetectors!=Phi.size()||nDetectors!=Theta.size()||nDetectors!=detID.size())
     throw std::runtime_error("The size of nexus data columns is not equal to each other");

   detStruct.resize(nDetectors);
   detOffset.resize(nDetectors);
   detList.resize(nDetectors);
   for(size_t i=0;i<nDetectors;i++)
   {
      detStruct[i].detID = detID[i];
      detStruct[i].l2    = L2[i];
      detStruct[i].theta = Theta[i];
      detStruct[i].phi   = Phi[i];
      detStruct[i].pressure = pressure;
      detStruct[i].wallThick = wallThickness;

      detOffset[i]  = float(delayTime[i]);
      detList[i] = detid_t(detID[i]);
    }
    
}

/**Read detector.dat information (see ) written in NeXus format 
 * @param hFile -- pointer to the opened NeXus file handle, opened at the group, which contains Libisis Detector information
 * @param  detStruct -- the vector of the DetectorInfo structures, describing the detectors to modify
 * @param  detType   -- the vector of the detector types, described in the algorithm description
 * @param  detOffset  -- the time delay for each detector's electronics, which should be corrected for.
 * @param detList  -- list of detectors
*/
void LoadDetectorInfo::readDetDotDatNXS(::NeXus::File *hFile, std::vector<detectorInfo> &detStruct, std::vector<int32_t>&detType,std::vector<float> &detOffset,
                                        std::vector<detid_t>&detList)
{

    std::vector<float> timeOffsets;
    std::vector<int32_t> detID;
    // read detector num and ID 
    hFile->readData<int32_t>("detID",detID);

    // read the detector's time offsets 
    hFile->readData<float>("timeOffsets",timeOffsets);

    int nDetectors = static_cast<int>(timeOffsets.size()/2);
    std::vector<float> detSphericalCoord;
    if(m_moveDets)
    {
        hFile->readData<float>("detSphericalCoord",detSphericalCoord);
    }
    else
    {
      detSphericalCoord.assign(3*nDetectors,FLT_MAX);
    }
    //We need He3 pressue and wall thikness
    std::vector<float> detPrWall;
    hFile->readData<float>("detPressureAndWall",detPrWall);


    if(nDetectors!=static_cast<int>(detSphericalCoord.size()/3)
            ||nDetectors!=static_cast<int>(detPrWall.size()/2)
            ||nDetectors!=static_cast<int>(detID.size()/2))
    {
        throw std::runtime_error("The size of nexus data columns is not equal to each other");
    }

   detStruct.resize(nDetectors);
   detOffset.resize(nDetectors);
   detType.resize(nDetectors);
   detList.resize(nDetectors);
   PARALLEL_FOR_NO_WSP_CHECK()
   for(int i=0;i<static_cast<int>(nDetectors);i++)
   {
      detStruct[i].detID = detID[2*i];
      detStruct[i].l2    = detSphericalCoord[3*i+0]; //L2,
      detStruct[i].theta = detSphericalCoord[3*i+1]; // Theta
      detStruct[i].phi   = detSphericalCoord[3*i+2]; // Phi

      detStruct[i].pressure = detPrWall[2*i+0]; //pressure;
      detStruct[i].wallThick =detPrWall[2*i+1]; // wallThickness;

      detOffset[i]  = timeOffsets[2*i];
      detType[i]    = detID[2*i+1];
      detList[i]   = detid_t(detID[2*i]);
    }
    
}

} // namespace DataHandling
} // namespace Mantid
