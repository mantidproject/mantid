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

  declareProperty("PreserveEvents", true,
    "If the InputWorkspace is an EventWorkspace, this will preserve the full event list (warning: this will use much more memory!).");
}

/** Executes the algorithm
 *
 *  @throw Exception::FileError If the grouping file cannot be opened or read successfully
 *  @throw runtime_error If unable to run one of the sub-algorithms successfully
 */
void AlignAndFocusPowder::exec()
{
  // retrieve the properties
  m_inputW = getProperty("InputWorkspace");
  m_eventW = boost::dynamic_pointer_cast<EventWorkspace>( m_inputW );
  if (m_eventW != NULL)
  {
    if (getProperty("PreserveEvents"))
    {
      //Input workspace is an event workspace. Use the other exec method
      this->execEvent();
      return;
    }
  }
  std::string instName = m_inputW->getInstrument()->getName();
  std::string calFileName=getProperty("CalFileName");
  OffsetsWorkspace_sptr offsetsWS = getProperty("OffsetsWorkspace");
  MatrixWorkspace_sptr maskWS = getProperty("MaskWorkspace");
  GroupingWorkspace_sptr groupWS = getProperty("GroupingWorkspace");
  std::string params = getProperty("Params");

  // Get the input workspace
  if ((!offsetsWS || !maskWS || !groupWS) && !calFileName.empty())
  {
    // Load the .cal file
    IAlgorithm_sptr alg = createSubAlgorithm("LoadCalFile");
    alg->setPropertyValue("CalFilename", calFileName);
    alg->setProperty("InputWorkspace", m_inputW);
    if(groupWS) alg->setProperty<bool>("MakeGroupingWorkspace", false);
    if(offsetsWS) alg->setProperty<bool>("MakeOffsetsWorkspace", false);
    if(maskWS)alg->setProperty<bool>("MakeMaskWorkspace", false);
    alg->setProperty<std::string>("WorkspaceName", instName);
    alg->executeAsSubAlg();
    groupWS = alg->getProperty("OutputGroupingWorkspace");
    offsetsWS = alg->getProperty("OutputOffsetsWorkspace");
    maskWS = alg->getProperty("OutputMaskWorkspace");
    AnalysisDataService::Instance().addOrReplace(instName+"_group", groupWS);
    AnalysisDataService::Instance().addOrReplace(instName+"_offsets", offsetsWS);
    AnalysisDataService::Instance().addOrReplace(instName+"_mask", maskWS);
  }

  API::IAlgorithm_sptr maskAlg = createSubAlgorithm("MaskDetectors");
  maskAlg->setProperty("Workspace", m_inputW);
  maskAlg->setProperty("MaskedWorkspace", instName+"_mask");
  maskAlg->executeAsSubAlg();
  m_outputW = maskAlg->getProperty("Workspace");

  API::IAlgorithm_sptr alignAlg = createSubAlgorithm("AlignDetectors");
  alignAlg->setProperty("InputWorkspace", m_outputW);
  alignAlg->setProperty("OffsetsWorkspace", instName+"_offsets");
  alignAlg->executeAsSubAlg();
  m_outputW = alignAlg->getProperty("OutputWorkspace");

  API::IAlgorithm_sptr focusAlg = createSubAlgorithm("DiffractionFocussing");
  focusAlg->setProperty("InputWorkspace", m_outputW);
  focusAlg->setProperty("GroupingWorkspace", instName+"_group");
  focusAlg->setProperty("PreserveEvents", true);
  focusAlg->executeAsSubAlg();
  m_outputW = focusAlg->getProperty("OutputWorkspace");

  API::IAlgorithm_sptr convertAlg = createSubAlgorithm("ConvertUnits");
  convertAlg->setProperty("InputWorkspace", m_outputW);
  convertAlg->setProperty("Target","TOF");
  convertAlg->executeAsSubAlg();
  m_outputW = convertAlg->getProperty("OutputWorkspace");

  API::IAlgorithm_sptr rebinAlg = createSubAlgorithm("Rebin");
  rebinAlg->setProperty("InputWorkspace", m_outputW);
  rebinAlg->setProperty("Params",params);
  rebinAlg->setProperty("PreserveEvents", false);
  rebinAlg->executeAsSubAlg();
  m_outputW = rebinAlg->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace",m_outputW);

}

/** Executes the algorithm
 *
 *  @throw Exception::FileError If the grouping file cannot be opened or read successfully
 *  @throw runtime_error If unable to run one of the sub-algorithms successfully
 */
void AlignAndFocusPowder::execEvent()
{
  // retrieve the properties
  std::string instName = m_inputW->getInstrument()->getName();
  std::string calFileName=getProperty("CalFileName");
  OffsetsWorkspace_sptr offsetsWS = getProperty("OffsetsWorkspace");
  MatrixWorkspace_sptr maskWS = getProperty("MaskWorkspace");
  GroupingWorkspace_sptr groupWS = getProperty("GroupingWorkspace");
  std::string params = getProperty("Params");

  // Get the input workspace
  if ((!offsetsWS || !maskWS || !groupWS) && !calFileName.empty())
  {
    // Load the .cal file
    IAlgorithm_sptr alg = createSubAlgorithm("LoadCalFile");
    alg->setPropertyValue("CalFilename", calFileName);
    alg->setProperty("InputWorkspace", m_inputW);
    if(groupWS) alg->setProperty<bool>("MakeGroupingWorkspace", false);
    if(offsetsWS) alg->setProperty<bool>("MakeOffsetsWorkspace", false);
    if(maskWS)alg->setProperty<bool>("MakeMaskWorkspace", false);
    alg->setProperty<std::string>("WorkspaceName", instName);
    alg->executeAsSubAlg();
    groupWS = alg->getProperty("OutputGroupingWorkspace");
    offsetsWS = alg->getProperty("OutputOffsetsWorkspace");
    maskWS = alg->getProperty("OutputMaskWorkspace");
    AnalysisDataService::Instance().addOrReplace(instName+"_group", groupWS);
    AnalysisDataService::Instance().addOrReplace(instName+"_offsets", offsetsWS);
    AnalysisDataService::Instance().addOrReplace(instName+"_mask", maskWS);
  }

  API::IAlgorithm_sptr maskAlg = createSubAlgorithm("MaskDetectors");
  maskAlg->setProperty("Workspace", m_inputW);
  maskAlg->setProperty("MaskedWorkspace", instName+"_mask");
  maskAlg->executeAsSubAlg();
  m_outputW = maskAlg->getProperty("Workspace");

  API::IAlgorithm_sptr alignAlg = createSubAlgorithm("AlignDetectors");
  alignAlg->setProperty("InputWorkspace", m_outputW);
  alignAlg->setProperty("OffsetsWorkspace", instName+"_offsets");
  alignAlg->executeAsSubAlg();
  MatrixWorkspace_sptr m_outputW = alignAlg->getProperty("OutputWorkspace");

  API::IAlgorithm_sptr focusAlg = createSubAlgorithm("DiffractionFocussing");
  focusAlg->setProperty("InputWorkspace", m_outputW);
  focusAlg->setProperty("GroupingWorkspace", instName+"_group");
  focusAlg->setProperty("PreserveEvents", true);
  focusAlg->executeAsSubAlg();
  m_outputW = focusAlg->getProperty("OutputWorkspace");

  API::IAlgorithm_sptr convertAlg = createSubAlgorithm("ConvertUnits");
  convertAlg->setProperty("InputWorkspace", m_outputW);
  convertAlg->setProperty("Target","TOF");
  convertAlg->executeAsSubAlg();
  m_outputW = convertAlg->getProperty("OutputWorkspace");

  API::IAlgorithm_sptr rebinAlg = createSubAlgorithm("Rebin");
  rebinAlg->setProperty("InputWorkspace", m_outputW);
  rebinAlg->setProperty("Params",params);
  rebinAlg->setProperty("PreserveEvents", true);
  rebinAlg->executeAsSubAlg();
  m_outputW = rebinAlg->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace",m_outputW);

}

} // namespace Algorithm
} // namespace Mantid
