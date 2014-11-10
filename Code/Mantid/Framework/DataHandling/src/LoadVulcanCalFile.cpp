#include "MantidDataHandling/LoadVulcanCalFile.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/System.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include <fstream>
#include <Poco/Path.h>

#include <sstream>
#include <fstream>

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>


using Mantid::Geometry::Instrument_const_sptr;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

using namespace std;

namespace Mantid
{
namespace DataHandling
{
  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(LoadVulcanCalFile)
  
  // Number of detectors per module/bank
  const size_t NUMBERDETECTORPERMODULE = 1232;
  // Number of reserved detectors per module/bank
  const size_t NUMBERRESERVEDPERMODULE = 1250;

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  LoadVulcanCalFile::LoadVulcanCalFile() : m_doAlignEventWS(false)
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  LoadVulcanCalFile::~LoadVulcanCalFile()
  {
  }


  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void LoadVulcanCalFile::init()
  {
    // LoadVulcanCalFile::getInstrument3WaysInit(this);

    declareProperty(new FileProperty("OffsetFilename", "", FileProperty::Load, ".dat"),
                    "Path to the VULCAN offset file. ");

    vector<string> groupoptions;
    groupoptions.push_back("6Modules");
    groupoptions.push_back("2Banks");
    groupoptions.push_back("1Bank");

    declareProperty("Grouping", "6Modules", boost::make_shared<ListValidator<string> >(groupoptions),
                    "Choices to output group workspace for 1 bank, 2 banks or 6 modules. ");

    declareProperty(new FileProperty("BadPixelFilename", "", FileProperty::OptionalLoad, ".dat"),
                    "Path to the VULCAN bad pixel file. ");

    declareProperty(new PropertyWithValue<std::string>("WorkspaceName", "", Direction::Input),
        "The base of the output workspace names. Names will have '_group', '_offsets', '_mask' appended to them.");

    // Effective geometry: bank IDs
    declareProperty(new ArrayProperty<int>("BankIDs"), "Bank IDs for the effective detectors. "
                    "Must cover all banks in the definition. ");

    // Effective geometry: DIFCs
    declareProperty(new ArrayProperty<double>("EffectiveDIFCs"), "DIFCs for effective detectors. ");

    // Effective geometry: 2theta
    declareProperty(new ArrayProperty<double>("Effective2Thetas"), "2 thetas for effective detectors. ");

    // These is the properties for testing purpose only!
    declareProperty(new WorkspaceProperty<EventWorkspace>("EventWorkspace", "", Direction::InOut, PropertyMode::Optional),
                    "Optional input/output EventWorkspace to get aligned by offset file. "
                    "It serves as a verifying tool, and will be removed after test. ");
  }


  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void LoadVulcanCalFile::exec()
  {
    // Process input properties
    processInOutProperites();

    // Grouping workspace
    setupGroupingWorkspace();

    // Mask workspace
    setupMaskWorkspace();

    // Genreate Offset workspace
    generateOffsetsWorkspace();

    // Output
    if (m_doAlignEventWS)
      setProperty("EventWorkspace", m_eventWS);

  }


  //----------------------------------------------------------------------------------------------
  /** Process input and output
    */
  void LoadVulcanCalFile::processInOutProperites()
  {
    // Input
    m_offsetFilename = getPropertyValue("OffsetFilename");
    m_badPixFilename = getPropertyValue("BadPixelFilename");

    string WorkspaceName = getPropertyValue("WorkspaceName");
    if (WorkspaceName.empty())
      throw std::invalid_argument("Must specify WorkspaceName.");

    // Get intrument
    m_instrument = getInstrument();

    // Grouping
    string grouptypestr = getPropertyValue("Grouping");
    size_t numeffbanks = 6;
    if (grouptypestr.compare("6Modules") == 0)
    {
      m_groupingType = VULCAN_OFFSET_BANK;
    }
    else if (grouptypestr.compare("2Banks") == 0)
    {
      m_groupingType = VULCAN_OFFSET_MODULE;
    }
    else if (grouptypestr.compare("1Bank") == 0)
    {
      m_groupingType = VULCAN_OFFSET_STACK;
    }
    else
    {
      stringstream ess;
      ess << "Group type " << grouptypestr << " is not supported. ";
      throw runtime_error(ess.str());
    }

    // Effective L and 2thetas
    vector<int> vec_bankids = getProperty("BankIDs");
    vector<double> vec_difcs = getProperty("EffectiveDIFCs");
    vector<double> vec_2thetas = getProperty("Effective2Thetas");
    if (vec_bankids.size() != numeffbanks || vec_difcs.size() != numeffbanks
        || vec_2thetas.size() != numeffbanks)
    {
      std::stringstream ess;
      ess << "Number of items of BankIDs (" << vec_bankids.size() << "), EffectiveDIFCs (" << vec_difcs.size()
          << ") and Effective2Thetas (" << vec_2thetas.size() << ") must be " << numeffbanks
          << " in mode '" << grouptypestr << "'! ";
      throw runtime_error(ess.str());
    }

    for (size_t i = 0; i < numeffbanks; ++i)
    {
      int bankid = vec_bankids[i];
      double difc = vec_difcs[i];
      double theta = 0.5*vec_2thetas[i];
      double effl = difc/(252.777 * 2.0 * sin(theta / 180. * M_PI));

      m_effLTheta.insert(make_pair(bankid, make_pair(effl, theta)));
    }

    // Create offset workspace
    std::string title = Poco::Path(m_offsetFilename).getFileName();
    m_tofOffsetsWS = OffsetsWorkspace_sptr(new OffsetsWorkspace(m_instrument));
    m_offsetsWS = OffsetsWorkspace_sptr(new OffsetsWorkspace(m_instrument));
    m_offsetsWS->setTitle(title);

    // Create mask workspace/bad pixel
    std::string masktitle = Poco::Path(m_badPixFilename).getFileName();
    m_maskWS = boost::make_shared<MaskWorkspace>(m_instrument);
    m_maskWS->setTitle(masktitle);

    // Set properties for these file
    m_offsetsWS->mutableRun().addProperty("Filename",m_offsetFilename);

    declareProperty(new WorkspaceProperty<OffsetsWorkspace>(
                      "OutputOffsetsWorkspace", WorkspaceName + "_offsets", Direction::Output),
                    "Set the the output OffsetsWorkspace. ");
    setProperty("OutputOffsetsWorkspace", m_offsetsWS);

    m_tofOffsetsWS->mutableRun().addProperty("Filename",m_offsetFilename);
    declareProperty(new WorkspaceProperty<OffsetsWorkspace>(
                      "OutputTOFOffsetsWorkspace", WorkspaceName + "_TOF_offsets", Direction::Output),
                    "Set the the (TOF) output OffsetsWorkspace. ");
    setProperty("OutputTOFOffsetsWorkspace", m_tofOffsetsWS);

    // mask workspace
    declareProperty(new WorkspaceProperty<MaskWorkspace>(
                      "OutputMaskWorkspace", WorkspaceName + "_mask", Direction::Output),
                    "Set the output MaskWorkspace. ");
    m_maskWS->mutableRun().addProperty("Filename", m_badPixFilename);
    setProperty("OutputMaskWorkspace", m_maskWS);

    // Extra event workspace as a verification tool
    m_eventWS = getProperty("EventWorkspace");
    if (m_eventWS)
      m_doAlignEventWS = true;
    else
      m_doAlignEventWS = false;

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Set up grouping workspace
    */
  void LoadVulcanCalFile::setupGroupingWorkspace()
  {
    // Get the right group option for CreateGroupingWorkspace
    string groupdetby = "";
    switch (m_groupingType)
    {
      case VULCAN_OFFSET_BANK:
        groupdetby = "bank";
        break;

      case VULCAN_OFFSET_MODULE:
        groupdetby = "Group";
        break;

      case VULCAN_OFFSET_STACK:
        groupdetby = "All";
        break;

      default:
        throw runtime_error("Grouping type is not supported. ");
        break;
    }

    // Calling algorithm CreateGroupingWorkspace
    IAlgorithm_sptr creategroupws = createChildAlgorithm("CreateGroupingWorkspace", -1, -1, true);
    creategroupws->initialize();

    creategroupws->setProperty("InstrumentName", "VULCAN");
    creategroupws->setProperty("GroupDetectorsBy", groupdetby);

    creategroupws->execute();
    if (!creategroupws->isExecuted())
      throw runtime_error("Unable to create grouping workspace.");

    m_groupWS = creategroupws->getProperty("OutputWorkspace");

    // Set title
    m_groupWS->setTitle(groupdetby);

    // Output
    string WorkspaceName = getPropertyValue("WorkspaceName");
    declareProperty(new WorkspaceProperty<GroupingWorkspace>(
                      "OutputGroupingWorkspace", WorkspaceName + "_group", Direction::Output),
                    "Set the output GroupingWorkspace. ");
    m_groupWS->mutableRun().addProperty("Filename", m_offsetFilename);
    setProperty("OutputGroupingWorkspace", m_groupWS);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Set up masking workspace
    */
  void LoadVulcanCalFile::setupMaskWorkspace()
  {

    // Skip if bad pixel file is not given
    if (m_badPixFilename.size() == 0) return;

    // Open file
    std::ifstream maskss(m_badPixFilename.c_str());
    if (!maskss.is_open())
    {
      g_log.warning("Bad pixel file cannot be read.");
      return;
    }

    string line;
    while (std::getline(maskss, line))
    {
      boost::algorithm::trim(line);
      if (!line.empty())
      {
        // Get the bad pixel's detector ID.  One per line
        stringstream liness(line);
        try
        {
          int pixelid;
          liness >> pixelid;

          // Set mask
          m_maskWS->setValue(pixelid, 1.0);
        }
        catch (const std::invalid_argument& e)
        {
          g_log.debug() << "Unable to parse line " << line << ".  Error message: " << e.what() << "\n";
          continue;
        }
      }
    }
    maskss.close();

    // Mask workspace index
    std::ostringstream msg;
    for (size_t i = 0; i < m_maskWS->getNumberHistograms(); ++i)
    {
      if (m_maskWS->readY(i)[0] > 0.5)
      {
        m_maskWS->maskWorkspaceIndex(i);
        m_maskWS->dataY(i)[0] = 1.0;
        msg << "Spectrum " << i << " is masked. DataY = " << m_maskWS->readY(i)[0] << "\n";
      }

    }
    g_log.information(msg.str());

    return;
  }


  //----------------------------------------------------------------------------------------------
  /** Generate offset workspace
    */
  void LoadVulcanCalFile::generateOffsetsWorkspace()
  {
    map<detid_t, double> map_detoffset;

    // Read offset file
    readOffsetFile(map_detoffset);

    // Generate map among PIDs, workspace indexes and offsets
    processOffsets(map_detoffset);

    // Convert the input EventWorkspace if necessary
    if (m_doAlignEventWS) alignEventWorkspace();

    // Convert to Mantid offset values
    convertOffsets();

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Read VULCAN's offset file
    */
  void LoadVulcanCalFile::readOffsetFile(std::map<detid_t, double>& map_detoffset)
  {
    // Read file
    ifstream infile(m_offsetFilename.c_str());
    if (!infile.is_open())
    {
      stringstream errss;
      errss << "Input offset file " << m_offsetFilename << " cannot be opened.";
      throw runtime_error(errss.str());
    }

    string line;
    while (std::getline(infile, line))
    {
      std::istringstream iss(line);
      int pid;
      double offset;
      if (!(iss >> pid >> offset)) continue;
      detid_t detid = static_cast<detid_t>(pid);
      map_detoffset.insert(make_pair(detid, offset));
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Process offsets by generating maps
    * Output: Offset workspace : 10^(xi_0 + xi_1 + xi_2)
    */
  void LoadVulcanCalFile::processOffsets(std::map<detid_t, double> map_detoffset)
  {
    size_t numspec = m_tofOffsetsWS->getNumberHistograms();

    // Map from Mantid instrument to VULCAN offset
    map<detid_t, size_t> map_det2index;
    for (size_t i = 0; i < numspec; ++i)
    {
      Geometry::IDetector_const_sptr det = m_tofOffsetsWS->getDetector(i);
      detid_t tmpid = det->getID();

      // Map between detector ID and workspace index
      map_det2index.insert(make_pair(tmpid, i));
    }

    // Map from VULCAN offset to Mantid instrument: Validate
    std::set<int> set_bankID;
    map<detid_t, pair<bool, int> > map_verify; // key: detector ID, value: flag to have a match, bank ID
    for (map<detid_t, double>::iterator miter = map_detoffset.begin(); miter != map_detoffset.end(); ++miter)
    {
      detid_t pid = miter->first;
      map<detid_t, size_t>::iterator fiter = map_det2index.find(pid);
      if (fiter == map_det2index.end())
      {
        map_verify.insert(make_pair(pid, make_pair(false, -1)));
      }
      else
      {
        size_t wsindex = fiter->second;
        // Get bank ID from instrument tree
        Geometry::IDetector_const_sptr det = m_tofOffsetsWS->getDetector(wsindex);
        Geometry::IComponent_const_sptr parent = det->getParent();
        string pname = parent->getName();

        vector<string> terms;
        boost::split(terms, pname, boost::is_any_of("("));
        vector<string> terms2;
        boost::split(terms2, terms[0], boost::is_any_of("bank"));
        int bank = atoi(terms2.back().c_str());
        set_bankID.insert(bank);

        map_verify.insert(make_pair(pid, make_pair(true, bank)));
      }
    }

    // Verify
    static const size_t arr[] = {21, 22, 23, 26, 27, 28};
    vector<size_t> vec_banks(arr, arr + sizeof(arr) / sizeof(arr[0]) );

    for (size_t i = 0; i < vec_banks.size(); ++i)
    {
      size_t bankindex = vec_banks[i];
      for (size_t j = 0; j < NUMBERDETECTORPERMODULE; ++j)
      {
        detid_t detindex = static_cast<detid_t>(bankindex * NUMBERRESERVEDPERMODULE + j);
        map<detid_t, pair<bool, int> >::iterator miter = map_verify.find(detindex);
        if (miter == map_verify.end())
          throw runtime_error("It cannot happen!");
        bool exist = miter->second.first;
        int tmpbank = miter->second.second;
        if (!exist)
          throw runtime_error("Define VULCAN offset pixel is not defined in Mantid.");
        if (tmpbank != static_cast<int>(bankindex))
          throw runtime_error("Bank ID does not match!");
      }
    }

    // Get the global correction
    std::set<int>::iterator biter;
    g_log.information() << "Number of bankds to process = " << set_bankID.size() << "\n";
    map<int, double> map_bankLogCorr;
    for (biter = set_bankID.begin(); biter != set_bankID.end(); ++biter)
    {
      // Locate inter bank and inter pack correction (log)
      int bankid = *biter;
      double globalfactor = 0.;
      map<detid_t, double>::iterator offsetiter;

      // Inter-bank correction
      if (m_groupingType != VULCAN_OFFSET_BANK)
      {
        detid_t interbank_detid = static_cast<detid_t>((bankid+1)*NUMBERRESERVEDPERMODULE) - 2;

        g_log.information() << "Find inter-bank correction for bank " << bankid << " for special detid "
                            << interbank_detid << ".\n";

        offsetiter = map_detoffset.find(interbank_detid);
        if (offsetiter == map_detoffset.end()) throw runtime_error("It cannot happen!");
        double interbanklogcorr = offsetiter->second;

        globalfactor += interbanklogcorr;
      }

      // Inter-module correction
      if (m_groupingType == VULCAN_OFFSET_STACK)
      {
        g_log.information() << "Find inter-module correction for bank " << bankid << ".\n";

        detid_t intermodule_detid = static_cast<detid_t>((bankid+1)*NUMBERRESERVEDPERMODULE) - 1;
        offsetiter = map_detoffset.find(intermodule_detid);
        if (offsetiter == map_detoffset.end()) throw runtime_error("It cannot happen!");
        double intermodulelogcorr = offsetiter->second;

        globalfactor += intermodulelogcorr;
      }

      map_bankLogCorr.insert(make_pair(bankid, globalfactor));
    }

    // Calcualte the offset for each detector (log now still)
    map<detid_t, double>::iterator offsetiter;
    map<int, double>::iterator bankcorriter;
    for (size_t iws = 0; iws < numspec; ++iws)
    {
      detid_t detid = m_tofOffsetsWS->getDetector(iws)->getID();
      offsetiter = map_detoffset.find(detid);
      if (offsetiter == map_detoffset.end()) throw runtime_error("It cannot happen!");

      int bankid = static_cast<int>(detid)/static_cast<int>(NUMBERRESERVEDPERMODULE);
      bankcorriter = map_bankLogCorr.find(bankid);
      if (bankcorriter == map_bankLogCorr.end()) throw runtime_error("It cannot happen!");

      double offset = offsetiter->second + bankcorriter->second;
      m_tofOffsetsWS->dataY(iws)[0] = pow(10., offset);
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Align the input EventWorkspace
    */
  void LoadVulcanCalFile::alignEventWorkspace()
  {
    g_log.notice("Align input EventWorkspace.");

    size_t numberOfSpectra = m_eventWS->getNumberHistograms();
    if (numberOfSpectra != m_tofOffsetsWS->getNumberHistograms())
      throw runtime_error("Number of histograms are different!");

    PARALLEL_FOR_NO_WSP_CHECK()
    for (int64_t i = 0; i < int64_t(numberOfSpectra); ++i)
    {
      PARALLEL_START_INTERUPT_REGION

      // Compute the conversion factor
      double factor = m_tofOffsetsWS->readY(i)[0];

      //Perform the multiplication on all events
      m_eventWS->getEventList(i).convertTof(1./factor);

      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Translate the VULCAN's offset to Mantid
    * Input Offset workspace : 10^(xi_0 + xi_1 + xi_2)
    *
    * This is the rigorous way to calcualte 2theta
    * detPos -= samplePos;
    * l2 = detPos.norm();
    * halfcosTwoTheta = detPos.scalar_prod(beamline)/(l2*beamline_norm);
    * sinTheta=sqrt(0.5-halfcosTwoTheta);
    *
    * 6 pixels in each bank to be focussed on
    * // (detid == 26870 || detid == 28120 || detid == 29370 ||
    * //  detid == 33120 || detid == 34370 || detid == 35620)
    *
    */
  void LoadVulcanCalFile::convertOffsets()
  {
    size_t numspec = m_tofOffsetsWS->getNumberHistograms();

    // Instrument parameters
    double l1;
    Kernel::V3D beamline, samplePos;
    double beamline_norm;

    m_instrument->getInstrumentParameters(l1,beamline,beamline_norm, samplePos);
    g_log.debug() << "Beam line = " << beamline.X() << ", " << beamline.Y() << ", "
                  << beamline.Z() << "\n";

    // FIXME - The simple version of the algorithm to calculate 2theta is used here.
    //         A check will be made to raise exception if the condition is not met to use the simple version.
    double s_r, s_2theta, s_phi;
    s_r = s_2theta = s_phi = 0.;
    samplePos.spherical(s_r, s_2theta, s_phi);
    if (fabs(beamline.X()) > 1.0E-20 || fabs(beamline.Y()) > 1.0E-20 || s_r > 1.0E-20)
      throw runtime_error("Source is not at (0, 0, Z) or sample is not at (0, 0, 0).  "
                          "The simple version to calcualte detector's 2theta fails on this situation.");

    map<int, pair<double, double> >::iterator mfiter;
    for (size_t iws = 0; iws < numspec; ++iws)
    {
      // Get detector's information including bank belonged to and geometry parameters
      Geometry::IDetector_const_sptr det = m_tofOffsetsWS->getDetector(iws);
      V3D detPos = det->getPos();

      detid_t detid = det->getID();
      int bankid = detid/static_cast<int>(NUMBERRESERVEDPERMODULE);

      double l2, twotheta, phi;
      detPos.getSpherical(l2, twotheta, phi);

      // Get effective
      mfiter = m_effLTheta.find(bankid);
      if (mfiter == m_effLTheta.end()) throw runtime_error("Effective DIFC and 2theta information is missed. ");

      double effL = mfiter->second.first;
      double effTheta = mfiter->second.second;
      double totL = l1 + l2;

      // Calcualte converted offset
      double vuloffset = m_tofOffsetsWS->readY(iws)[0];
      double manoffset = (totL * sin(twotheta * 0.5 * M_PI / 180.)) / (effL * sin(effTheta * M_PI / 180.)) / vuloffset - 1.;
      m_offsetsWS->dataY(iws)[0] = manoffset;
    }


    return;
  }


  //----------------------------------------------------------------------------------------------
  /** Get a pointer to an instrument in one of 3 ways: InputWorkspace, InstrumentName, InstrumentFilename
    */
  Geometry::Instrument_const_sptr LoadVulcanCalFile::getInstrument()
  {
    // Set up name
    std::string InstrumentName("VULCAN");

    // Get the instrument
    Instrument_const_sptr inst;

    Algorithm_sptr childAlg = createChildAlgorithm("LoadInstrument",0.0,0.2);
    MatrixWorkspace_sptr tempWS(new Workspace2D());
    childAlg->setProperty<MatrixWorkspace_sptr>("Workspace", tempWS);
    childAlg->setPropertyValue("InstrumentName", InstrumentName);
    childAlg->setProperty("RewriteSpectraMap", false);
    childAlg->executeAsChildAlg();
    inst = tempWS->getInstrument();

    return inst;
  }




  //-----------------------------------------------------------------------
  /** Reads the calibration file.
   *
   * @param calFileName :: path to the old .cal file
   * @param groupWS :: optional, GroupingWorkspace to fill. Must be initialized to the right instrument.
   * @param offsetsWS :: optional, OffsetsWorkspace to fill. Must be initialized to the right instrument.
   * @param maskWS :: optional, masking-type workspace to fill. Must be initialized to the right instrument.
   */
  void LoadVulcanCalFile::readCalFile(const std::string& calFileName,
      GroupingWorkspace_sptr groupWS, OffsetsWorkspace_sptr offsetsWS, MaskWorkspace_sptr maskWS)
  {
    bool doGroup = bool(groupWS);
    bool doOffsets = bool(offsetsWS);
    bool doMask = bool(maskWS);

    bool hasUnmasked(false);
    bool hasGrouped(false);

    if (!doOffsets && !doGroup && !doMask)
      throw std::invalid_argument("You must give at least one of the grouping, offsets or masking workspaces.");

    std::ifstream grFile(calFileName.c_str());
    if (!grFile)
    {
      throw std::runtime_error("Unable to open calibration file " + calFileName);
    }

    size_t numErrors = 0;

    detid2index_map detID_to_wi;
    if (doMask)
    {
      detID_to_wi = maskWS->getDetectorIDToWorkspaceIndexMap();
    }

    // not all of these should be doubles, but to make reading work read as double then recast to int
    int n,udet,select,group;
    double n_d, udet_d, offset, select_d, group_d;

    std::string str;
    while(getline(grFile,str))
    {
      if (str.empty() || str[0] == '#') continue;
      std::istringstream istr(str);

      // read in everything as double then cast as appropriate
      istr >> n_d >> udet_d >> offset >> select_d >> group_d;
      n = static_cast<int>(n_d);
      udet = static_cast<int>(udet_d);
      select = static_cast<int>(select_d);
      group = static_cast<int>(group_d);

      if (doOffsets)
      {
        if (offset <= -1.) // should never happen
        {
          std::stringstream msg;
          msg << "Encountered offset = " << offset << " at index " << n << " for udet = " << udet
              << ". Offsets must be greater than -1.";
          throw std::runtime_error(msg.str());
        }

        try
        {
          offsetsWS->setValue(udet, offset);
        }
        catch (std::invalid_argument &)
        {
          numErrors++;
        }
      }

      if (doGroup)
      {
        try
        {
          groupWS->setValue(udet, double(group) );
          if ((!hasGrouped) && (group > 0))
            hasGrouped = true;
        }
        catch (std::invalid_argument &)
        {
          numErrors++;
        }
      }

      if (doMask)
      {
        detid2index_map::const_iterator it = detID_to_wi.find(udet);
        if (it != detID_to_wi.end())
        {
          size_t wi = it->second;

          if (select <= 0)
          {
            // Not selected, then mask this detector
            maskWS->maskWorkspaceIndex(wi);
            maskWS->dataY(wi)[0] = 1.0;
          }
          else
          {
            // Selected, set the value to be 0
            maskWS->dataY(wi)[0] = 0.0;
            if (!hasUnmasked)
              hasUnmasked = true;
          }

        }
        else
        {
          // Could not find the UDET.
          numErrors++;
        }
      }
    }

    // Warn about any errors

    if (numErrors > 0)
      Logger("LoadVulcanCalFile").warning() << numErrors << " errors (invalid Detector ID's) found when reading .cal file '" << calFileName << "'.\n";
    if (doGroup && (!hasGrouped))
      Logger("LoadVulcanCalFile").warning() << "'" << calFileName << "' has no spectra grouped\n";
    if (doMask && (!hasUnmasked))
      Logger("LoadVulcanCalFile").warning() << "'" << calFileName << "' masks all spectra\n";
  }


} // namespace Mantid
} // namespace DataHandling
