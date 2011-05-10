#include "MantidAlgorithms/AlignDetectors.h"
#include "MantidAlgorithms/DspacemaptoCal.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidGeometry/V3D.h"
#include "MantidKernel/BinaryFile.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"
#include <cmath>
#include <fstream>

using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace Algorithms
{
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(DspacemaptoCal)

/// Sets documentation strings for this algorithm
void DspacemaptoCal::initDocs()
{
  this->setWikiSummary("Creates a calibration file with offsets calculated from Dspacemap file. ");
  this->setOptionalMessage("Creates a calibration file with offsets calculated from Dspacemap file.");
}



//========================================================================
//========================================================================
/// (Empty) Constructor
DspacemaptoCal::DspacemaptoCal()
{
  this->useAlgorithm("LoadDspacemap");
  this->deprecatedDate("2011-05-10");
}

/// Destructor
DspacemaptoCal::~DspacemaptoCal()
{
}

//-----------------------------------------------------------------------
void DspacemaptoCal::init()
{
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
  MatrixWorkspace_sptr WS = getProperty("InputWorkspace");

  Algorithm_sptr childAlg = createSubAlgorithm("LoadDspacemap");
  childAlg->setProperty("InputWorkspace", WS);
  childAlg->setPropertyValue("FileType", getPropertyValue("FileType"));
  childAlg->setPropertyValue("Filename", getPropertyValue("DspacemapFile"));
  childAlg->executeAsSubAlg();
  OffsetsWorkspace_sptr offsetsWS = childAlg->getProperty("OutputWorkspace");

  childAlg = createSubAlgorithm("LoadCalFile");
  childAlg->setProperty("InputWorkspace", WS);
  childAlg->setPropertyValue("CalFilename", getPropertyValue("CalibrationFile"));
  childAlg->setProperty<bool>("MakeGroupingWorkspace", true);
  childAlg->setProperty<bool>("MakeOffsetsWorkspace", false);
  childAlg->setProperty<bool>("MakeMaskingWorkspace", false);
  childAlg->setPropertyValue("WorkspaceName", "__temp");
  childAlg->executeAsSubAlg();
  GroupingWorkspace_sptr groupWS = boost::dynamic_pointer_cast<GroupingWorkspace>(
      AnalysisDataService::Instance().retrieve("__temp_group"));
  AnalysisDataService::Instance().remove("__temp_group");

  childAlg = createSubAlgorithm("SaveCalFile");
  childAlg->setProperty<GroupingWorkspace_sptr>("GroupingWorkspace", groupWS);
  childAlg->setProperty<OffsetsWorkspace_sptr>("OffsetsWorkspace", offsetsWS);
  childAlg->setPropertyValue("MaskWorkspace", "");
  childAlg->setPropertyValue("Filename", getPropertyValue("CalibrationFile"));
  childAlg->executeAsSubAlg();

}





} // namespace Algorithms
} // namespace Mantid
