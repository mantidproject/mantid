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
#include "MantidGeometry/Instrument/ComponentHelper.h"
#include "LoadRaw/isisraw2.h"

#include <boost/algorithm/string/compare.hpp>
#include <nexus/NeXusFile.hpp>
#include <Poco/Path.h>

#include <fstream>

namespace Mantid
{
  namespace DataHandling
  {
    using namespace Kernel;
    using namespace API;
    using namespace Geometry;

    namespace
    {
      // Name of the offset parameter
      const char * DELAY_PARAM = "DelayTime";
      // Name of pressure parameter
      const char * PRESSURE_PARAM = "TubePressure";
      // Name of wall thickness parameter
      const char * THICKNESS_PARAM = "TubeThickness";
    }

    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(LoadDetectorInfo)

    /// Empty default constructor
    LoadDetectorInfo::LoadDetectorInfo()
    : Algorithm(), m_baseInstrument(), m_samplePos(), m_moveDets(false),
      m_workspace(), m_instDelta(-1.0), m_instPressure(-1.0), m_instThickness(-1.0)
    {
    }

    void LoadDetectorInfo::init()
    {

      declareProperty(new WorkspaceProperty<>("Workspace","",Direction::InOut),
                      "The name of the workspace to that the detector information will be loaded into");
      std::vector<std::string> exts;
      // each of these allowed extensions must be dealt with in exec() below
      exts.push_back(".dat");
      exts.push_back(".raw");
      exts.push_back(".sca");
      exts.push_back(".nxs");
      declareProperty(new FileProperty("DataFilename","", FileProperty::Load, exts),
          "A .raw,.dat,.nxs or .sca file that contains information about the detectors in the "
          "workspace. The description of Dat and nxs file format is provided below.");

      declareProperty("RelocateDets", false,
          "If true then the detectors are moved to the positions specified in the raw/dat/nxs file.",
          Direction::Input);
    }

    /// Sets documentation strings for this algorithm
    void LoadDetectorInfo::initDocs()
    {
      this->setWikiSummary("Loads delay times, tube pressures and tube wall thicknesses from a given file.");
      this->setOptionalMessage("Loads delay times, tube pressures and tube wall thicknesses from a given file.");
    }

    /**
     */
    void LoadDetectorInfo::exec()
    {
      cacheInputs();
      std::string filename = getPropertyValue("DataFilename");
      std::string ext = Poco::Path(filename).getExtension();
      if(boost::iequals(ext, "dat") || boost::iequals(ext, "sca"))
      {
        loadFromDAT(filename);
      }
      else if(boost::iequals(ext, "raw"))
      {
        loadFromRAW(filename);
      }
      else if(boost::iequals(ext, "nxs"))
      {
        loadFromIsisNXS(filename);
      }
      else
      {
        throw std::invalid_argument("Unknown file type with extension=." + ext);
      }
    }

    /**
     * Cache frequently accessed user input
     */
    void LoadDetectorInfo::cacheInputs()
    {
      m_workspace = getProperty("Workspace");
      m_moveDets = getProperty("RelocateDets");

      // Cache base instrument
      m_baseInstrument = m_workspace->getInstrument()->baseInstrument();
      Geometry::IComponent_const_sptr sample = m_workspace->getInstrument()->getSample();
      if( sample ) m_samplePos = sample->getPos();

      // cache values of instrument level parameters so we only change then if they are different
      const auto & pmap = m_workspace->constInstrumentParameters();
      //delay
      auto param = pmap.get(m_baseInstrument->getComponentID(), DELAY_PARAM);
      if(param) m_instDelta = param->value<double>();
      // pressure
      param = pmap.get(m_baseInstrument->getComponentID(), PRESSURE_PARAM);
      if(param) m_instPressure = param->value<double>();
      // thickness
      param = pmap.get(m_baseInstrument->getComponentID(), THICKNESS_PARAM);
      if(param) m_instThickness = param->value<double>();

    }

    /**
     * Full format is defined in doc text
     * @param filename A full path to the input DAT file
     */
    void LoadDetectorInfo::loadFromDAT(const std::string & filename)
    {
      std::ifstream datFile(filename.c_str());
      if(!datFile)
      {
        throw Exception::FileError("Unable to access dat file", filename);
      }

      std::string line;
      // skip 3 lines of header info
      for(int i = 0; i < 3; ++i) getline(datFile, line);

      // start loop over file
      auto & pmap = m_workspace->instrumentParameters();
      while(getline(datFile, line))
      {
        if(line.empty() || line[0] == '#') continue;

        std::istringstream is(line);
        detid_t detID(0);
        int code(0);
        float droppedFloat(0.0f);
        float delta(0.0f), l2(0.0f), theta(0.0f), phi(0.0f);
        is >> detID >> delta >> l2 >> code >> theta >> phi;
        // offset value is be subtracted so store negative
        delta *= -1.0f;

        IDetector_const_sptr det;
        try
        {
          det = m_baseInstrument->getDetector(detID);
        }
        catch(Exception::NotFoundError&)
        {
          continue;
        }
        if(det->isMonitor() || code == 1) continue;

        // drop 10 float columns
        for(int i = 0; i < 10; ++i) is >> droppedFloat;

        // pressure, wall thickness
        float pressure(0.0), thickness(0.0);
        is >> pressure >> thickness;

        updateParameterMap(pmap, det, l2, theta, phi, delta, pressure, thickness);
      }
    }

    /**
     * @param filename A full path to the input RAW file
     */
    void LoadDetectorInfo::loadFromRAW(const std::string & filename)
    {
      ISISRAW2 iraw;
      if (iraw.readFromFile(filename.c_str(), false) != 0)
      {
        throw Exception::FileError("Unable to access raw file:" , filename);
      }

      const int numDets = iraw.i_det;
      const int numUserTables = iraw.i_use;
      int pressureTabNum(0), thicknessTabNum(0);
      if(numUserTables == 10)
      {
        pressureTabNum = 7;
        thicknessTabNum = 8;
      }
      else if(numUserTables == 14)
      {
        pressureTabNum = 11;
        thicknessTabNum = 12;
      }
      else
      {
        throw std::invalid_argument("RAW file contains unexpected number of user tables="
            + boost::lexical_cast<std::string>(numUserTables) + ". Expected 10 or 14.");
      }

      // Is ut01 (=phi) present? Sometimes an array is present but has wrong values
      // e.g.all 1.0 or all 2.0
      bool phiPresent = (iraw.ut[0]!= 1.0 && iraw.ut[0] !=2.0);

      // Start loop over detectors
      auto & pmap = m_workspace->instrumentParameters();
      for(int i = 0; i < numDets; ++i)
      {
        detid_t detID = static_cast<detid_t>(iraw.udet[i]);
        int code = iraw.code[i];
        IDetector_const_sptr det;
        try
        {
          det = m_baseInstrument->getDetector(detID);
        }
        catch(Exception::NotFoundError&)
        {
          continue;
        }
        if(det->isMonitor() || code == 1) continue;

        // Positions
        float l2 = iraw.len2[i];
        float theta = iraw.tthe[i];
        float phi = (phiPresent ? iraw.ut[i] : 0.0f);

        // Parameters
        float delta = iraw.delt[i];
        // offset value is be subtracted so store negative
        delta *= -1.0f;
        // pressure, wall thickness
        float pressure = iraw.ut[i + pressureTabNum*numDets];
        float thickness = iraw.ut[i + thicknessTabNum*numDets];

        updateParameterMap(pmap, det, l2, theta, phi, delta, pressure, thickness);
      }
    }

    /**
     *
     * @param filename filename A full path to the input RAW file
     */
    void LoadDetectorInfo::loadFromIsisNXS(const std::string & filename)
    {
      ::NeXus::File nxsFile(filename, NXACC_READ); // will throw if file can't be opened

      // two types of file:
      //   - new type entry per detector
      //   - old libisis with single pressure, thickness entry for whole file

      // hold data read from file
      DetectorInfo detInfo;

      std::map<std::string, std::string> entries;
      nxsFile.getEntries(entries);
      if(entries.find("full_reference_detector") != entries.end())
      {
        nxsFile.openGroup("full_reference_detector","NXIXTdetector");
        readLibisisNxs(nxsFile, detInfo);
        nxsFile.closeGroup();
      }
      else if(entries.find("detectors.dat") != entries.end())
      {
        nxsFile.openGroup("detectors.dat","NXEntry");
        readNXSDotDat(nxsFile, detInfo);
        nxsFile.closeGroup();
      }
      else
      {
        throw std::invalid_argument("Unknown NeXus file type");
      }
      nxsFile.close();

      // Start loop over detectors
      auto & pmap = m_workspace->instrumentParameters();
      int numDets = static_cast<int>(detInfo.ids.size());
      for(int i = 0; i < numDets; ++i)
      {
        detid_t detID = detInfo.ids[i];
        int code = detInfo.codes[i];
        IDetector_const_sptr det;
        try
        {
          det = m_baseInstrument->getDetector(detID);
        }
        catch(Exception::NotFoundError&)
        {
          continue;
        }
        if(det->isMonitor() || code == 1) continue;

        // Positions
        double l2 = detInfo.l2[i];
        double theta = detInfo.theta[i];
        double phi = detInfo.phi[i];

        // Parameters
        double delta = detInfo.delays[i];
        // offset value is be subtracted so store negative
        delta *= -1.0;
        // pressure, wall thickness
        double pressure = detInfo.pressures[i];
        double thickness = detInfo.thicknesses[i];

        updateParameterMap(pmap, det, l2, theta, phi, delta, pressure, thickness);
      }
    }

    /**
     *
     * @param nxsFile A reference to the open NeXus fileIt should be opened at the
     *                "full_reference_detector" group
     * @param detInfo A reference to the struct that will hold the data from the file
     */
    void LoadDetectorInfo::readLibisisNxs(::NeXus::File & nxsFile, DetectorInfo & detInfo) const
    {
      nxsFile.readData<int32_t>("det_no", detInfo.ids);
      nxsFile.readData<int32_t>("det_type", detInfo.codes);
      nxsFile.readData<double>("delay_time", detInfo.delays);
      const size_t numDets = detInfo.ids.size();

      if(m_moveDets)
      {
        nxsFile.readData<double>("L2", detInfo.l2);
        nxsFile.readData<double>("theta", detInfo.theta);
        nxsFile.readData<double>("phi", detInfo.phi);
      }
      else
      {
        // these will get ignored
        detInfo.l2.resize(numDets, -1.0);
        detInfo.theta.resize(numDets, -1.0);
        detInfo.phi.resize(numDets, -1.0);
      }

      // pressure & wall thickness are global here
      double pressure = -1.0;
      double thickness = -1.0;
      nxsFile.openGroup("det_he3", "NXIXTdet_he3");
      nxsFile.readData<double>("gas_pressure", pressure);
      nxsFile.readData<double>("wall_thickness", thickness);
      nxsFile.closeGroup();
      if(pressure <= 0.0)
      {
        g_log.warning("The data file does not contain correct He3 pressure, "
                      "default value of 10 bar is used instead");
        pressure = 10.0;
      }
      if(thickness <= 0.0)
      {
        g_log.warning("The data file does not contain correct detector's wall "
                      "thickness, default value of 0.8mm is used instead");
        thickness = 0.0008;
      }
      detInfo.pressures.resize(numDets, pressure);
      detInfo.thicknesses.resize(numDets, thickness);
    }

    /**
     *
     * @param nxsFile A reference to the open NeXus fileIt should be opened at the
     *                "detectors.dat" group
     * @param detInfo A reference to the struct that will hold the data from the file
     */
    void LoadDetectorInfo::readNXSDotDat(::NeXus::File & nxsFile, DetectorInfo & detInfo) const
    {
      std::vector<int32_t> fileIDs;
      nxsFile.readData<int32_t>("detID", fileIDs); // containts both ids & codes
      std::vector<float> fileOffsets;
      nxsFile.readData<float>("timeOffsets", fileOffsets);
      const size_t numDets = fileOffsets.size()/2;

      std::vector<float> detCoords;
      if(m_moveDets)
      {
        nxsFile.readData<float>("detSphericalCoord", detCoords);
      }
      else
      {
        detCoords.resize(3*numDets, -1.0f);
      }

      //pressure & wall thickness
      std::vector<float> pressureAndWall;
      nxsFile.readData<float>("detPressureAndWall", pressureAndWall);

      if( numDets != fileIDs.size()/2 || numDets != detCoords.size()/3 ||
          numDets != pressureAndWall.size()/2 )
      {
        std::ostringstream os;
        os << "The sizes of NeXus data columns are inconsistent in detectors.dat.\n"
           << "detIDs=" << fileIDs.size() << ", offsets=" << fileOffsets.size()
           << ", pressure & thickness=" << pressureAndWall.size() << "\n";
        throw std::runtime_error(os.str());
      }

      detInfo.ids.resize(numDets);
      detInfo.codes.resize(numDets);
      detInfo.delays.resize(numDets);
      detInfo.l2.resize(numDets);  detInfo.theta.resize(numDets);  detInfo.phi.resize(numDets);
      detInfo.pressures.resize(numDets);  detInfo.thicknesses.resize(numDets);

      PARALLEL_FOR_NO_WSP_CHECK()
      for(int i=0; i < static_cast<int>(numDets);i++)
      {
        detInfo.ids[i] = fileIDs[2*i];
        detInfo.codes[i] = fileIDs[2*i+1];
        detInfo.delays[i] = fileOffsets[2*i];

        detInfo.l2[i] = detCoords[3*i+0]; //L2,
        detInfo.theta[i] = detCoords[3*i+1]; // Theta
        detInfo.phi[i] = detCoords[3*i+2]; // Phi

        detInfo.pressures[i] = pressureAndWall[2*i]; //pressure;
        detInfo.thicknesses[i] = pressureAndWall[2*i+1]; // wallThickness;
      }
    }

    /**
     *
     * @param pmap A reference to the ParameterMap instance to update
     * @param det A pointer to the detector whose parameters should be updated
     * @param l2 The new l2 value
     * @param theta The new theta value
     * @param phi The new phi value
     * @param delay The new delay time
     * @param pressure The new pressure value
     * @param thickness The new thickness value
     */
    void LoadDetectorInfo::updateParameterMap(Geometry::ParameterMap & pmap,
                                              const Geometry::IDetector_const_sptr & det,
                                              const double l2, const double theta, const double phi,
                                              const double delay, const double pressure,
                                              const double thickness) const
    {
      // store detector params that are different to instrument level
      if(fabs(delay - m_instDelta) > 1e-06) pmap.addDouble(det->getComponentID(), DELAY_PARAM, delay);
      if(fabs(pressure - m_instPressure) > 1e-06) pmap.addDouble(det->getComponentID(), PRESSURE_PARAM, pressure);
      if(fabs(thickness - m_instThickness) > 1e-06) pmap.addDouble(det->getComponentID(), THICKNESS_PARAM, thickness);

      // move
      if(m_moveDets)
      {
        V3D newPos;
        newPos.spherical(l2, theta, phi);
        // The sample position may not be at 0,0,0
        newPos += m_samplePos;
        ComponentHelper::moveComponent(*det, pmap, newPos, ComponentHelper::Absolute);
      }
    }

  }
}

