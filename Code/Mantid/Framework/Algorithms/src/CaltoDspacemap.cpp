//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/AlignDetectors.h"
#include "MantidAlgorithms/CaltoDspacemap.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidGeometry/V3D.h"
#include "MantidKernel/BinaryFile.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"
#include <cmath>
#include <fstream>

using namespace Mantid::Geometry;

namespace Mantid
{
namespace Algorithms
{
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(CaltoDspacemap)

/// Sets documentation strings for this algorithm
void CaltoDspacemap::initDocs()
{
  this->setWikiSummary("Creates a Dspacemap file from calibration file with offsets calculated. ");
  this->setOptionalMessage("Creates a Dspacemap file from calibration file with offsets calculated. ");
}


using namespace Kernel;
using namespace API;
using namespace DataObjects;
using Geometry::IInstrument_const_sptr;
using DataObjects::EventList;
using DataObjects::EventWorkspace;
using DataObjects::EventWorkspace_sptr;
using DataObjects::EventWorkspace_const_sptr;




//========================================================================
//========================================================================
/// (Empty) Constructor
CaltoDspacemap::CaltoDspacemap()
{
  this->useAlgorithm("LoadCalFile, then SaveDspacemap");
  this->deprecatedDate("2011-05-12");
}

/// Destructor
CaltoDspacemap::~CaltoDspacemap()
{
}

//-----------------------------------------------------------------------
void CaltoDspacemap::init()
{
  declareProperty(
    new WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace","",Direction::Input),
    "A workspace with units of TOF" );

  declareProperty(new FileProperty("CalibrationFile", "", FileProperty::Load, ".cal"),
     "The CalFile on input contains the offsets");

  declareProperty(new FileProperty("DspacemapFile", "", FileProperty::Save, ".dat"),
     "The DspacemapFile on output contains the d-space mapping");

  declareProperty("PadDetID", 300000, "Pad Data to this number of pixels");

}





//-----------------------------------------------------------------------
/** Executes the algorithm
 *  @throw Exception::FileError If the calibration file cannot be opened and read successfully
 *  @throw Exception::InstrumentDefinitionError If unable to obtain the source-sample distance
 */
void CaltoDspacemap::exec()
{
  const MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  const std::string DFileName = getProperty("DspacemapFile");
  const std::string calFileName = getProperty("CalibrationFile");

  progress(0.0,"Reading calibration file");
  IAlgorithm_sptr alg = createSubAlgorithm("LoadCalFile", 0.0, 0.5);
  alg->setProperty("InputWorkspace", inputWS);
  alg->setPropertyValue("CalFilename", calFileName);
  alg->setProperty<bool>("MakeGroupingWorkspace", false);
  alg->setProperty<bool>("MakeOffsetsWorkspace", true);
  alg->setProperty<bool>("MakeMaskWorkspace", false);
  alg->setPropertyValue("WorkspaceName", "temp");
  alg->executeAsSubAlg();
  OffsetsWorkspace_sptr offsetsWS;
  offsetsWS = alg->getProperty("OutputOffsetsWorkspace");

  progress(0.5,"Saving dspacemap file");
  alg = createSubAlgorithm("SaveDspacemap", 0.5, 1.0);
  alg->setPropertyValue("Filename", DFileName);
  alg->setProperty<int>("PadDetID", getProperty("PadDetID"));
  alg->setProperty("InputWorkspace", offsetsWS);
  alg->executeAsSubAlg();
}

} // namespace Algorithms
} // namespace Mantid
