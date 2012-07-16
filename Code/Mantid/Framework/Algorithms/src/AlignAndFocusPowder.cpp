/*WIKI* 

*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/AlignAndFocusPowder.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/LoadCalFile.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/System.h"

using Mantid::Geometry::Instrument_const_sptr;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(AlignAndFocusPowder)

/// Constructor
AlignAndFocusPowder::AlignAndFocusPowder() : API::Algorithm(), API::DeprecatedAlgorithm()
{
  this->useAlgorithm("AlignAndFocusPowder version 2");
}

/// Sets documentation strings for this algorithm
void AlignAndFocusPowder::initDocs()
{
  this->setWikiSummary("Algorithm to focus powder diffraction data into a number of histograms according to a grouping scheme defined in a [[CalFile]]. ");
  this->setOptionalMessage("Algorithm to focus powder diffraction data into a number of histograms according to a grouping scheme defined in a CalFile.");
}


using namespace Kernel;
using API::WorkspaceProperty;
using API::MatrixWorkspace_sptr;
using API::MatrixWorkspace;
using API::FileProperty;

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void AlignAndFocusPowder::init()
{
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input),
    "The input workspace" );

  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output),
    "The result of diffraction focussing of InputWorkspace" );

  declareProperty("Params","","The binning parameters");

  declareProperty(new FileProperty("CalFileName", "", FileProperty::OptionalLoad, ".cal"),
		  "The name of the CalFile with offset, masking, and grouping data" );

  declareProperty(new WorkspaceProperty<GroupingWorkspace>("GroupingWorkspace","",Direction::Input, PropertyMode::Optional),
    "Optional: An GroupingWorkspace workspace giving the grouping info.");

  declareProperty(new WorkspaceProperty<OffsetsWorkspace>("OffsetsWorkspace","",Direction::Input, PropertyMode::Optional),
    "Optional: An OffsetsWorkspace workspace giving the detector calibration values.");

  declareProperty(new WorkspaceProperty<MatrixWorkspace>("MaskWorkspace","",Direction::Input, PropertyMode::Optional),
    "Optional: An Workspace workspace giving which detectors are masked.");

}

/** Executes the algorithm
 *
 *  @throw Exception::FileError If the grouping file cannot be opened or read successfully
 *  @throw runtime_error If unable to run one of the sub-algorithms successfully
 */
void AlignAndFocusPowder::exec()
{
  // retrieve the properties
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  std::string calFileName=getProperty("CalFileName");
  OffsetsWorkspace_sptr offsetsWS = getProperty("OffsetsWorkspace");
  MatrixWorkspace_sptr maskWS = getProperty("MaskWorkspace");
  GroupingWorkspace_sptr groupWS = getProperty("GroupingWorkspace");

  // Get the input workspace
  if ((!offsetsWS || !maskWS || !groupWS) && !calFileName.empty())
  {
    // Load the .cal file
    IAlgorithm_sptr alg = createSubAlgorithm("LoadCalFile");
    alg->setPropertyValue("CalFilename", calFileName);
    alg->setProperty("InputWorkspace", inputWS);
    if(groupWS) alg->setProperty<bool>("MakeGroupingWorkspace", false);
    if(offsetsWS) alg->setProperty<bool>("MakeOffsetsWorkspace", false);
    if(maskWS)alg->setProperty<bool>("MakeMaskWorkspace", false);
    std::string instName = inputWS->getInstrument()->getName();
    alg->setProperty<std::string>("WorkspaceName", instName);
    alg->executeAsSubAlg();
    groupWS = alg->getProperty("OutputGroupingWorkspace");
    offsetsWS = alg->getProperty("OutputOffsetsWorkspace");
    maskWS = alg->getProperty("OutputMaskWorkspace");
    AnalysisDataService::Instance().add(instName+"_group", groupWS);
    AnalysisDataService::Instance().add(instName+"_offsets", offsetsWS);
    AnalysisDataService::Instance().add(instName+"_mask", maskWS);
  }

  std::string params = getProperty("Params");
  API::IAlgorithm_sptr rebinAlg = createSubAlgorithm("Rebin");
  rebinAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputWS);
  rebinAlg->setProperty("Params",params);
  rebinAlg->setProperty("PreserveEvents", true);
  rebinAlg->executeAsSubAlg();
  MatrixWorkspace_sptr outputWS = rebinAlg->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace",outputWS);

}

} // namespace Algorithm
} // namespace Mantid
