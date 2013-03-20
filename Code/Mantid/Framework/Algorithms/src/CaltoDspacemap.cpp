/*WIKI* 
This is the inverse of the DspacemaptoCal algorithm.  The detector offset file created by this algorithm are in the form created by the ARIEL software. The offsets are a correction to the dSpacing values and are applied during the conversion from time-of-flight to dSpacing as follows:

:<math> d = \frac{h}{m_N} \frac{t.o.f.}{L_{tot} sin \theta} (1+ \rm{offset})</math>


== Additional Property Information ==

{| border="1" cellpadding="5" cellspacing="0"
!Name
!Description
|-
|InputWorkspace
|The name of the input workspace containing instrument geometry.  The [[instrument]] associated with the workspace must be fully defined because detector, source & sample position are needed.
|-
|CalibrationFile
|The [[CalFile]] containing the groups from CreateCalFileByNames or CreateDummyCalFile is input.  Output [[CalFile]] contains both groups and offsets calculated from Dspacemap file.
|-
|DspacemapFile
|The binary Dspacemap file [.dat] for ISAW.
|-
|PadDetID
|Pad Dspacemap file to this number of pixels.
|}

*WIKI*/
/*WIKI_USAGE*
'''Python'''
    LoadEmptyInstrument("POWGEN_Definition.xml","POWGEN")
    CaltoDspacemap("POWGEN","PG3.cal", "PG3.dat")

'''C++'''
    IAlgorithm* alg1 = FrameworkManager::Instance().createAlgorithm("LoadEmptyInstrument");
    alg1->setPropertyValue("Filename", "POWGEN_Definition.xml");
    alg1->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", "POWGEN");
    alg1->execute();
    IAlgorithm* alg2 = FrameworkManager::Instance().createAlgorithm("DspacemaptoCal");
    alg2->setProperty<MatrixWorkspace_sptr>("InputWorkspace", "POWGEN");
    alg2->setPropertyValue("CalibrationFile", "PG3.cal");
    alg2->setPropertyValue("DspacemapFile", "PG3.dat");
    alg2->execute();
*WIKI_USAGE*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/AlignDetectors.h"
#include "MantidAlgorithms/CaltoDspacemap.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/V3D.h"
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
  this->setWikiSummary("Creates Dspacemap file from a calibration file with offsets.");

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
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  const std::string DFileName = getProperty("DspacemapFile");
  const std::string calFileName = getProperty("CalibrationFile");

  progress(0.0,"Reading calibration file");
  IAlgorithm_sptr alg = createChildAlgorithm("LoadCalFile", 0.0, 0.5, true);
  alg->setProperty("InputWorkspace", inputWS);
  alg->setPropertyValue("CalFilename", calFileName);
  alg->setProperty<bool>("MakeGroupingWorkspace", false);
  alg->setProperty<bool>("MakeOffsetsWorkspace", true);
  alg->setProperty<bool>("MakeMaskWorkspace", false);
  alg->setPropertyValue("WorkspaceName", "temp");
  alg->executeAsChildAlg();
  OffsetsWorkspace_sptr offsetsWS;
  offsetsWS = alg->getProperty("OutputOffsetsWorkspace");

  progress(0.5,"Saving dspacemap file");
  alg = createChildAlgorithm("SaveDspacemap", 0.5, 1.0, true);
  alg->setPropertyValue("DspacemapFile", DFileName);
  alg->setProperty<int>("PadDetID", getProperty("PadDetID"));
  alg->setProperty("InputWorkspace", offsetsWS);
  alg->executeAsChildAlg();
}

} // namespace Algorithms
} // namespace Mantid
