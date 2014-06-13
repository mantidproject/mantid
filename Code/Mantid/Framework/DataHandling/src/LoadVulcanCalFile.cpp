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
#include <fstream>
#include <Poco/Path.h>

#include <sstream>
#include <fstream>

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
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  LoadVulcanCalFile::LoadVulcanCalFile()
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
  }


  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void LoadVulcanCalFile::exec()
  {
    // Process input properties
    std::string offsetfilename = getPropertyValue("OffsetFilename");

#if 0

    bool MakeGroupingWorkspace = getProperty("MakeGroupingWorkspace");
    bool MakeOffsetsWorkspace = getProperty("MakeOffsetsWorkspace");
    bool MakeMaskWorkspace = getProperty("MakeMaskWorkspace");
#endif

    string WorkspaceName = getPropertyValue("WorkspaceName");
    if (WorkspaceName.empty())
      throw std::invalid_argument("Must specify WorkspaceName.");

    // Get intrument
    Instrument_const_sptr inst = getInstrument();

    GroupingWorkspace_sptr groupWS;
    MaskWorkspace_sptr maskWS;

    // Title of all workspaces = the file without path
    std::string title = Poco::Path(offsetfilename).getFileName();

    // Make offset workspace
    OffsetsWorkspace_sptr offsetsWS = OffsetsWorkspace_sptr(new OffsetsWorkspace(inst));
    offsetsWS->setTitle(title);
    declareProperty(new WorkspaceProperty<OffsetsWorkspace>("OutputOffsetsWorkspace", WorkspaceName + "_offsets", Direction::Output),
            "Set the the output OffsetsWorkspace, if any.");
    offsetsWS->mutableRun().addProperty("Filename",offsetfilename);
    setProperty("OutputOffsetsWorkspace", offsetsWS);
    readOffsetFile(offsetsWS, offsetfilename);

#if 0
    // Initialize all required workspaces.
    if (MakeGroupingWorkspace)
    {
      groupWS = GroupingWorkspace_sptr(new GroupingWorkspace(inst));
      groupWS->setTitle(title);
      declareProperty(new WorkspaceProperty<GroupingWorkspace>("OutputGroupingWorkspace", WorkspaceName + "_group", Direction::Output),
              "Set the the output GroupingWorkspace, if any.");
      groupWS->mutableRun().addProperty("Filename",CalFilename);
      setProperty("OutputGroupingWorkspace", groupWS);
    }





    if (MakeMaskWorkspace)
    {
      maskWS = MaskWorkspace_sptr(new MaskWorkspace(inst));
      maskWS->setTitle(title);
      declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputMaskWorkspace", WorkspaceName + "_mask", Direction::Output),
              "Set the the output MaskWorkspace, if any.");
      maskWS->mutableRun().addProperty("Filename",CalFilename);
      setProperty("OutputMaskWorkspace", maskWS);
    }

    LoadVulcanCalFile::readCalFile(CalFilename, groupWS, offsetsWS, maskWS);
#endif
  }

  //----------------------------------------------------------------------------------------------
  /** Read VULCAN's offset file
    */
  void LoadVulcanCalFile::readOffsetFile(DataObjects::OffsetsWorkspace_sptr offsetws, std::string offsetfilename)
  {

    ifstream infile(offsetfilename);
    if (!infile.is_open()) throw runtime_error("Input offset file cannot be opened.");

    string line;
    while (std::getline(infile, line))
    {
      std::istringstream iss(line);
      int pid;
      double offset;
      if (!(iss >> pid >> offset)) continue;
    }
    return;
  }



  //----------------------------------------------------------------------------------------------
  /** Get a pointer to an instrument in one of 3 ways: InputWorkspace, InstrumentName, InstrumentFilename
   * @param alg :: algorithm from which to get the property values.
   * */
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
