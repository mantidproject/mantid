#include "MantidAPI/FileProperty.h"
#include "MantidDataHandling/SaveCalFile.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidGeometry/IInstrument.h"
#include "MantidKernel/System.h"
#include <cmath>
#include <fstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

namespace Mantid
{
namespace DataHandling
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(SaveCalFile)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SaveCalFile::SaveCalFile()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  SaveCalFile::~SaveCalFile()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void SaveCalFile::initDocs()
  {
    this->setWikiSummary("Saves a 5-column ASCII .cal file from up to 3 workspaces: a GroupingWorkspace, OffsetsWorkspace and/or MaskingWorkspace.");
    this->setOptionalMessage("Saves a 5-column ASCII .cal file from up to 3 workspaces: a GroupingWorkspace, OffsetsWorkspace and/or MaskingWorkspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void SaveCalFile::init()
  {
    declareProperty(new WorkspaceProperty<GroupingWorkspace>("GroupingWorkspace","",Direction::Input, true),
        "Optional: An GroupingWorkspace workspace giving the grouping info.");

    declareProperty(new WorkspaceProperty<OffsetsWorkspace>("OffsetsWorkspace","",Direction::Input, true),
        "Optional: An OffsetsWorkspace workspace giving the detector calibration values.");

    declareProperty(new WorkspaceProperty<>("MaskWorkspace","",Direction::Input, true),
        "Optional: An Workspace workspace giving which detectors are masked.");

    declareProperty(new FileProperty("Filename", "", FileProperty::Save, ".cal"),
        "Path to the .cal file that will be created.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void SaveCalFile::exec()
  {
    GroupingWorkspace_sptr groupWS = getProperty("GroupingWorkspace");
    OffsetsWorkspace_sptr offsetsWS = getProperty("OffsetsWorkspace");
    MatrixWorkspace_sptr maskWS = getProperty("MaskWorkspace");
    std::string Filename = getPropertyValue("Filename");

    // Do the saving
    SaveCalFile::saveCalFile(Filename, groupWS, offsetsWS, maskWS);
  }


  //-----------------------------------------------------------------------
  /** Reads the calibration file.
   *
   * @param calFileName :: path to the old .cal file
   * @param groupWS :: optional, GroupingWorkspace to save. Will be 0 if not specified.
   * @param offsetsWS :: optional, OffsetsWorkspace to save. Will be 0.0 if not specified.
   * @param maskWS :: optional, masking-type workspace to save. Will be 1 (selected) if not specified.
   */
  void SaveCalFile::saveCalFile(const std::string& calFileName,
      GroupingWorkspace_sptr groupWS, OffsetsWorkspace_sptr offsetsWS, MatrixWorkspace_sptr maskWS)
  {
    IInstrument_sptr inst;

    bool doGroup = false;
    if (groupWS) { doGroup = true; inst = groupWS->getInstrument(); }
    bool doOffsets = false;
    if (offsetsWS) { doOffsets = true; inst = offsetsWS->getInstrument(); }
    bool doMask = false;
    if (maskWS) { doMask = true; inst = maskWS->getInstrument(); }

    if (!inst)
      throw std::invalid_argument("You must give at least one of the grouping, offsets or masking workspaces.");

    // Header of the file
    std::ofstream fout(calFileName.c_str());
    fout <<"# Calibration file for instrument " << inst->getName() << " written on " << DateAndTime::get_current_time().to_ISO8601_string() << ".\n";
    fout <<"# Format: number    UDET         offset    select    group\n";

    // Get all the detectors
    std::map<int, Geometry::IDetector_sptr> allDetectors;
    inst->getDetectors(allDetectors);
    int number=0;

    std::map<int, Geometry::IDetector_sptr>::const_iterator it;
    for (it = allDetectors.begin(); it != allDetectors.end(); it++)
    {
      int detectorID = it->first;
      Geometry::IDetector_sptr det = it->second;

      //Find the offset, if any
      double offset = 0.0;
      if (doOffsets)
        offset = offsetsWS->getValue(detectorID, 0.0);

      //Find the group, if any
      int group = 1;
      if (doGroup)
        group = int(groupWS->getValue(detectorID, 0.0));

      // Find the selection, if any
      bool selected = true;
      if (doMask)
        selected = !maskWS->getInstrument()->getDetector(detectorID)->isMasked();

      //if(group > 0)
        fout << std::fixed << std::setw(9) << number <<
        std::fixed << std::setw(15) << detectorID <<
        std::fixed << std::setprecision(7) << std::setw(15)<< offset <<
        std::fixed << std::setw(8) << (selected?1:0) <<
        std::fixed << std::setw(8) << group  << "\n";

       number++;
    }

  }


} // namespace Mantid
} // namespace DataHandling

