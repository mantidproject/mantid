/*WIKI* 

The detector offset file created by this algorithm are in the form created by the ARIEL software. The offsets are a correction to the dSpacing values and are applied during the conversion from time-of-flight to dSpacing as follows:

:<math> d = \frac{h}{m_N} \frac{t.o.f.}{L_{tot} sin \theta} (1+ \rm{offset})</math>


==Usage==
'''Python'''
    LoadEmptyInstrument("POWGEN_Definition.xml","POWGEN")
    CreateCalFileByNames("POWGEN","PG3.cal","Group1,Group2,Group3,Group4")
    DspacemaptoCal("POWGEN","PG3_D1370_dspacemap_2010_09_12.dat","PG3.cal")

'''C++'''
    IAlgorithm* alg1 = FrameworkManager::Instance().createAlgorithm("LoadEmptyInstrument");
    alg1->setPropertyValue("Filename", "POWGEN_Definition.xml");
    alg1->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", "POWGEN");
    alg1->execute();
    IAlgorithm* alg2 = FrameworkManager::Instance().createAlgorithm("CreateCalFileByNames");
    alg2->setProperty<MatrixWorkspace_sptr>("InstrumentWorkspace", "POWGEN");
    alg2->setPropertyValue("GroupingFileName", "PG3.cal");
    alg2->setPropertyValue("GroupingNames", "Group1,Group2,Group3,Group4");
    alg2->execute();
    IAlgorithm* alg3 = FrameworkManager::Instance().createAlgorithm("DspacemaptoCal");
    alg3->setProperty<MatrixWorkspace_sptr>("InputWorkspace", "POWGEN");
    alg3->setPropertyValue("DspacemapFile", "PG3_D1370_dspacemap_2010_09_12.dat");
    alg3->setPropertyValue("CalibrationFile", "PG3.cal");
    alg3->execute();





*WIKI*/
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
#include "MantidKernel/V3D.h"
#include "MantidKernel/BinaryFile.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"
#include <cmath>
#include <fstream>
#include "MantidKernel/ListValidator.h"

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
  declareProperty("FileType", "POWGEN", boost::make_shared<StringListValidator>(propOptions),
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
  childAlg->setProperty<bool>("MakeMaskWorkspace", false);
  childAlg->setPropertyValue("WorkspaceName", "__temp");
  childAlg->executeAsSubAlg();
  GroupingWorkspace_sptr groupWS = childAlg->getProperty("OutputGroupingWorkspace");

  childAlg = createSubAlgorithm("SaveCalFile");
  childAlg->setProperty<GroupingWorkspace_sptr>("GroupingWorkspace", groupWS);
  childAlg->setProperty<OffsetsWorkspace_sptr>("OffsetsWorkspace", offsetsWS);
  childAlg->setPropertyValue("MaskWorkspace", "");
  childAlg->setPropertyValue("Filename", getPropertyValue("CalibrationFile"));
  childAlg->executeAsSubAlg();

}





} // namespace Algorithms
} // namespace Mantid
