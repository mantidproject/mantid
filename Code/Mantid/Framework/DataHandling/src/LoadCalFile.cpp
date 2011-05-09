#include "MantidAPI/Algorithm.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/LoadCalFile.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/IInstrument.h"
#include "MantidKernel/System.h"
#include <fstream>
#include "MantidDataObjects/SpecialWorkspace2D.h"

using Mantid::Geometry::IInstrument_sptr;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid
{
namespace DataHandling
{

  // Get the logger
  Kernel::Logger& LoadCalFile::g_log = Kernel::Logger::get("LoadCalFile");

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(LoadCalFile)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  LoadCalFile::LoadCalFile()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  LoadCalFile::~LoadCalFile()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void LoadCalFile::initDocs()
  {
    this->setWikiSummary("TODO: Enter a quick description of your algorithm.");
    this->setOptionalMessage("TODO: Enter a quick description of your algorithm.");
  }


  //----------------------------------------------------------------------------------------------
  /** For use by getInstrument3Ways, initializes the properties
   * @param alg :: algorithm to which to add the properties.
   * */
  void LoadCalFile::getInstrument3WaysInit(Algorithm * alg)
  {
    alg->declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input, true),
        "Optional: An input workspace with the instrument we want to use.");

    alg->declareProperty(new PropertyWithValue<std::string>("InstrumentName","",Direction::Input),
        "Optional: Name of the instrument to base the GroupingWorkspace on which to base the GroupingWorkspace.");

    alg->declareProperty(new FileProperty("InstrumentFilename", "", FileProperty::OptionalLoad, ".xml"),
        "Optional: Path to the instrument definition file on which to base the GroupingWorkspace.");
  }


  //----------------------------------------------------------------------------------------------
  /** Get a pointer to an instrument in one of 3 ways: InputWorkspace, InstrumentName, InstrumentFilename
   * @param alg :: algorithm from which to get the property values.
   * */
  Mantid::Geometry::IInstrument_sptr LoadCalFile::getInstrument3Ways(Algorithm * alg)
  {
    MatrixWorkspace_sptr inWS = alg->getProperty("InputWorkspace");
    std::string InputWorkspace = alg->getPropertyValue("InputWorkspace");
    std::string InstrumentName = alg->getPropertyValue("InstrumentName");
    std::string InstrumentFilename = alg->getPropertyValue("InstrumentFilename");

    // Some validation
    int numParams = 0;
    if (inWS) numParams++;
    if (!InstrumentName.empty()) numParams++;
    if (!InstrumentFilename.empty()) numParams++;

    if (numParams > 1)
      throw std::invalid_argument("You must specify exactly ONE way to get an instrument (workspace, instrument name, or IDF file). You specified more than one.");
    if (numParams == 0)
      throw std::invalid_argument("You must specify exactly ONE way to get an instrument (workspace, instrument name, or IDF file). You specified none.");

    // ---------- Get the instrument one of 3 ways ---------------------------
    IInstrument_sptr inst;
    if (inWS)
    {
      inst = inWS->getInstrument();
    }
    else
    {
      Algorithm_sptr childAlg = alg->createSubAlgorithm("LoadInstrument",0.0,0.2);
      MatrixWorkspace_sptr tempWS(new Workspace2D());
      childAlg->setProperty<MatrixWorkspace_sptr>("Workspace", tempWS);
      childAlg->setPropertyValue("Filename", InstrumentFilename);
      childAlg->setPropertyValue("InstrumentName", InstrumentName);
      childAlg->executeAsSubAlg();
      inst = tempWS->getInstrument();
    }

    return inst;
  }


  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void LoadCalFile::init()
  {
    LoadCalFile::getInstrument3WaysInit(this);

    declareProperty(new FileProperty("CalFilename", "", FileProperty::Load, ".cal"),
        "Path to the old-style .cal grouping/calibration file (multi-column ASCII). You must also specify the instrument.");

    declareProperty(new PropertyWithValue<bool>("MakeGroupingWorkspace",true,Direction::Input),
        "Set to true to create a GroupingWorkspace with called WorkspaceName_group.");

    declareProperty(new PropertyWithValue<bool>("MakeOffsetsWorkspace",true,Direction::Input),
        "Set to true to create a OffsetsWorkspace with called WorkspaceName_offsets.");

    declareProperty(new PropertyWithValue<bool>("MakeMaskingWorkspace",true,Direction::Input),
        "Set to true to create a MaskingWorkspace with called WorkspaceName_mask.");

    declareProperty(new PropertyWithValue<std::string>("WorkspaceName", "", Direction::Input),
        "The base of the output workspace names. Names will have '_group', '_offsets', '_mask' appended to them.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void LoadCalFile::exec()
  {
    std::string CalFilename = getPropertyValue("CalFilename");
    std::string WorkspaceName = getPropertyValue("WorkspaceName");
    bool MakeGroupingWorkspace = getProperty("MakeGroupingWorkspace");
    bool MakeOffsetsWorkspace = getProperty("MakeOffsetsWorkspace");
    bool MakeMaskingWorkspace = getProperty("MakeMaskingWorkspace");

    if (WorkspaceName.empty())
      throw std::invalid_argument("Must specify WorkspaceName.");

    IInstrument_sptr inst = LoadCalFile::getInstrument3Ways(this);

    GroupingWorkspace_sptr groupWS;
    OffsetsWorkspace_sptr offsetsWS;
    MatrixWorkspace_sptr maskWS;

    if (MakeGroupingWorkspace)
    {
      groupWS = GroupingWorkspace_sptr(new GroupingWorkspace(inst));
      AnalysisDataService::Instance().addOrReplace(WorkspaceName + "_group", groupWS);
    }

    if (MakeOffsetsWorkspace)
    {
      offsetsWS = OffsetsWorkspace_sptr(new OffsetsWorkspace(inst));
      AnalysisDataService::Instance().addOrReplace(WorkspaceName + "_offsets", offsetsWS);
    }

    if (MakeMaskingWorkspace)
    {
      maskWS = MatrixWorkspace_sptr(new SpecialWorkspace2D(inst));
      AnalysisDataService::Instance().addOrReplace(WorkspaceName + "_mask", maskWS);
    }

    LoadCalFile::readCalFile(CalFilename, groupWS, offsetsWS, maskWS);
  }


  //-----------------------------------------------------------------------
  /** Reads the calibration file.
   *
   * @param calFileName :: path to the old .cal file
   * @param groupWS :: optional, GroupingWorkspace to fill. Must be initialized to the right instrument.
   * @param offsetsWS :: optional, OffsetsWorkspace to fill. Must be initialized to the right instrument.
   * @param maskWS :: optional, masking-type workspace to fill. Must be initialized to the right instrument.
   */
  void LoadCalFile::readCalFile(const std::string& calFileName,
      GroupingWorkspace_sptr groupWS, OffsetsWorkspace_sptr offsetsWS, MatrixWorkspace_sptr maskWS)
  {
    bool doGroup = false;
    if (groupWS) doGroup = true;
    bool doOffsets = false;
    if (offsetsWS) doOffsets = true;
    bool doMask = false;
    if (maskWS) doMask = true;

    if (!doOffsets && !doGroup && !doMask)
      throw std::invalid_argument("You must give at least one of the grouping, offsets or masking workspaces.");

    std::ifstream grFile(calFileName.c_str());
    if (!grFile)
    {
      throw std::runtime_error("Unable to open calibration file " + calFileName);
    }

    size_t numErrors = 0;

    IndexToIndexMap * detID_to_wi;
    if (doMask)
    {
      detID_to_wi = maskWS->getDetectorIDToWorkspaceIndexMap( false );
    }

    std::string str;
    while(getline(grFile,str))
    {
      if (str.empty() || str[0] == '#') continue;
      std::istringstream istr(str);
      int n,udet,select,group;
      double offset;
      istr >> n >> udet >> offset >> select >> group;

      if (doOffsets)
      {
        if (offset < -1.) // should never happen
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
        catch (std::invalid_argument & e)
        {
          numErrors++;
        }
      }

      if (doGroup)
      {
        try
        {
          groupWS->setValue(udet, double(group) );
        }
        catch (std::invalid_argument & e)
        {
          numErrors++;
        }
      }

      if (doMask)
      {
        IndexToIndexMap::const_iterator it = detID_to_wi->find(udet);
        if (it != detID_to_wi->end())
        {
          int wi = it->second;
          if (select <= 0)
            maskWS->maskWorkspaceIndex(wi, 0.0);
          else
            maskWS->dataY(wi)[0] = 1.0;
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
      g_log.warning() << numErrors << " errors (invalid Detector ID's) found when reading .cal file '" << calFileName << "'.\n";

    if (doMask)
      delete detID_to_wi;

  }


} // namespace Mantid
} // namespace DataHandling

