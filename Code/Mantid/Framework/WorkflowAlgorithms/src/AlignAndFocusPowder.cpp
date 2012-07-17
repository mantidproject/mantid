/*WIKI* 

*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidWorkflowAlgorithms/AlignAndFocusPowder.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
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
namespace WorkflowAlgorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(AlignAndFocusPowder)

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
  declareProperty(new FileProperty("CalFileName", "", FileProperty::OptionalLoad, ".cal"),
		  "The name of the CalFile with offset, masking, and grouping data" );
  declareProperty(new WorkspaceProperty<GroupingWorkspace>("GroupingWorkspace","",Direction::Input, PropertyMode::Optional),
    "Optional: An GroupingWorkspace workspace giving the grouping info.");
  declareProperty(new WorkspaceProperty<OffsetsWorkspace>("OffsetsWorkspace","",Direction::Input, PropertyMode::Optional),
    "Optional: An OffsetsWorkspace workspace giving the detector calibration values.");
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("MaskWorkspace","",Direction::Input, PropertyMode::Optional),
    "Optional: An Workspace workspace giving which detectors are masked.");
  declareProperty("Params","-0.004","The binning parameters: Positive is linear bins, negative is logarithmic (Default:-0.004)");
  declareProperty("Dspacing", true,"Bin in Dspace. (Default true)");
  declareProperty("CropMin", 0.0, "Minimum for Cropping TOF or dspace axis. (Default 0.) ");
  declareProperty("CropMax", 0.0, "Maximum for Croping TOF or dspace axis. (Default 0.) ");
  declareProperty("PreserveEvents", true,
    "If the InputWorkspace is an EventWorkspace, this will preserve the full event list (warning: this will use much more memory!).");
  declareProperty("FilterBadPulses", true,
    "If the InputWorkspace is an EventWorkspace, filter bad pulses.");
  declareProperty("RemovePromptPulseWidth", 0.,
    "Width of events (in microseconds) near the prompt pulse to remove. 0 disables");
  declareProperty("CompressTolerance", 0.01,
    "Compress events (in microseconds) within this tolerance. (Default 0.01) ");
  declareProperty("FilterLogName", "",
    "Name of log used for filtering. (Default None) ");
  declareProperty("FilterLogMinimumValue", 0.0,
    "Events with log larger that this value will be included. (Default 0.0) ");
  declareProperty("FilterLogMaximumValue", 0.0,
    "Events with log smaller that this value will be included. (Default 0.0) ");
  declareProperty("UnwrapRef", 0., "Reference total flight path for frame unwrapping. Zero skips the correction");
  declareProperty("LowResRef", 0., "Reference DIFC for resolution removal. Zero skips the correction");
  declareProperty("CropWavelengthMin", 0., "Crop the data at this minimum wavelength. Overrides LowResRef.");
  declareProperty("TMin", 0.0, "Minimum for TOF or dspace axis. (Default 0.) ");
  declareProperty("TMax", 0.0, "Maximum for TOF or dspace axis. (Default 0.) ");


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
  bool dspace = getProperty("DSpacing");
  double xmin = getProperty("CropMin");
  double xmax = getProperty("CropMax");
  double LRef = getProperty("UnwrapRef");
  double DIFCref = getProperty("LowResRef");
  double minwl = getProperty("CropWavelengthMin");
  double tmin = getProperty("TMin");
  double tmax = getProperty("TMax");


  // Get the input workspace
  if ((!offsetsWS || !maskWS || !groupWS) && !calFileName.empty())
  {
    // Load the .cal file
    IAlgorithm_sptr alg = createSubAlgorithm("LoadCalFile");
    alg->setPropertyValue("CalFilename", calFileName);
    alg->setProperty("InputWorkspace", m_inputW);
    alg->setProperty<std::string>("WorkspaceName", instName);
    alg->executeAsSubAlg();
    groupWS = alg->getProperty("OutputGroupingWorkspace");
    offsetsWS = alg->getProperty("OutputOffsetsWorkspace");
    maskWS = alg->getProperty("OutputMaskWorkspace");
    AnalysisDataService::Instance().addOrReplace(instName+"_group", groupWS);
    AnalysisDataService::Instance().addOrReplace(instName+"_offsets", offsetsWS);
    AnalysisDataService::Instance().addOrReplace(instName+"_mask", maskWS);
  }

  if (xmin > 0. || xmax > 0.)
  {
	  API::IAlgorithm_sptr cropAlg = createSubAlgorithm("CropWorkspace");
	  cropAlg->setProperty("InputWorkspace", m_inputW);
	  if (xmin > 0.)cropAlg->setProperty("Xmin", xmin);
	  if (xmax > 0.)cropAlg->setProperty("Xmax", xmax);
	  cropAlg->executeAsSubAlg();
	  m_inputW = cropAlg->getProperty("OutputWorkspace");
  }

  API::IAlgorithm_sptr maskAlg = createSubAlgorithm("MaskDetectors");
  maskAlg->setProperty("Workspace", m_inputW);
  maskAlg->setProperty("MaskedWorkspace", instName+"_mask");
  maskAlg->executeAsSubAlg();
  m_outputW = maskAlg->getProperty("Workspace");

  if(!dspace)
  {
	  API::IAlgorithm_sptr rebinAlg = createSubAlgorithm("Rebin");
	  rebinAlg->setProperty("InputWorkspace", m_outputW);
	  rebinAlg->setProperty("Params",params);
	  rebinAlg->executeAsSubAlg();
	  m_outputW = rebinAlg->getProperty("OutputWorkspace");
	  setProperty("OutputWorkspace",m_outputW);
  }

  API::IAlgorithm_sptr alignAlg = createSubAlgorithm("AlignDetectors");
  alignAlg->setProperty("InputWorkspace", m_outputW);
  alignAlg->setProperty("OffsetsWorkspace", instName+"_offsets");
  alignAlg->executeAsSubAlg();
  m_outputW = alignAlg->getProperty("OutputWorkspace");

  if(LRef > 0. || minwl > 0. || DIFCref > 0.)
  {
          API::IAlgorithm_sptr convertAlg = createSubAlgorithm("ConvertUnits");
          convertAlg->setProperty("InputWorkspace", m_outputW);
          convertAlg->setProperty("Target","TOF");
          convertAlg->executeAsSubAlg();
          m_outputW = convertAlg->getProperty("OutputWorkspace");
  }

  if(LRef > 0.)
  {
	  API::IAlgorithm_sptr removeAlg = createSubAlgorithm("RemoveLowResTOF");
	  removeAlg->setProperty("InputWorkspace", m_outputW);
	  removeAlg->setProperty("LRef",LRef);
	  removeAlg->setProperty("Tmin",tmin);
	  removeAlg->setProperty("Tmax",tmax);
	  removeAlg->executeAsSubAlg();
	  m_outputW = removeAlg->getProperty("OutputWorkspace");
	  setProperty("OutputWorkspace",m_outputW);
  }

  if(minwl > 0.)
  {
	  API::IAlgorithm_sptr removeAlg = createSubAlgorithm("RemoveLowResTOF");
	  removeAlg->setProperty("InputWorkspace", m_outputW);
	  removeAlg->setProperty("MinWavelength",minwl);
	  removeAlg->setProperty("Tmin",tmin);
	  removeAlg->executeAsSubAlg();
	  m_outputW = removeAlg->getProperty("OutputWorkspace");
	  setProperty("OutputWorkspace",m_outputW);
  }
  else if(DIFCref > 0.)
  {
	  API::IAlgorithm_sptr removeAlg = createSubAlgorithm("RemoveLowResTOF");
	  removeAlg->setProperty("InputWorkspace", m_outputW);
	  removeAlg->setProperty("ReferenceDIFC",DIFCref);
	  removeAlg->setProperty("K",3.22);
	  removeAlg->setProperty("Tmin",tmin);
	  removeAlg->executeAsSubAlg();
	  m_outputW = removeAlg->getProperty("OutputWorkspace");
	  setProperty("OutputWorkspace",m_outputW);
  }

  if(LRef > 0. || minwl > 0. || DIFCref > 0.)
  {
          API::IAlgorithm_sptr convertAlg = createSubAlgorithm("ConvertUnits");
          convertAlg->setProperty("InputWorkspace", m_outputW);
          convertAlg->setProperty("Target","dSpacing");
          convertAlg->executeAsSubAlg();
          m_outputW = convertAlg->getProperty("OutputWorkspace");
  }

  if(dspace)
  {
	  API::IAlgorithm_sptr rebinAlg = createSubAlgorithm("Rebin");
	  rebinAlg->setProperty("InputWorkspace", m_outputW);
	  rebinAlg->setProperty("Params",params);
	  rebinAlg->executeAsSubAlg();
	  m_outputW = rebinAlg->getProperty("OutputWorkspace");
	  setProperty("OutputWorkspace",m_outputW);
  }

  API::IAlgorithm_sptr focusAlg = createSubAlgorithm("DiffractionFocussing");
  focusAlg->setProperty("InputWorkspace", m_outputW);
  focusAlg->setProperty("GroupingWorkspace", instName+"_group");
  focusAlg->setProperty("PreserveEvents", false);
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
  bool dspace = getProperty("DSpacing");
  double xmin = getProperty("CropMin");
  double xmax = getProperty("CropMax");
  double LRef = getProperty("UnwrapRef");
  double DIFCref = getProperty("LowResRef");
  double minwl = getProperty("CropWavelengthMin");
  double tmin = getProperty("TMin");
  double tmax = getProperty("TMax");
  bool filterBadPulses = getProperty("FilterBadPulses");
  double removePromptPulseWidth = getProperty("RemovePromptPulseWidth");
  double tolerance = getProperty("CompressTolerance");
  std::string filterName = getProperty("FilterLogName");
  double filterMin = getProperty("FilterLogMinimumValue");
  double filterMax = getProperty("FilterLogMaximumValue");

  // Get the input workspace
  if ((!offsetsWS || !maskWS || !groupWS) && !calFileName.empty())
  {
    // Load the .cal file
    IAlgorithm_sptr alg = createSubAlgorithm("LoadCalFile");
    alg->setPropertyValue("CalFilename", calFileName);
    alg->setProperty("InputWorkspace", m_inputW);
    alg->setProperty<std::string>("WorkspaceName", instName);
    alg->executeAsSubAlg();
    groupWS = alg->getProperty("OutputGroupingWorkspace");
    offsetsWS = alg->getProperty("OutputOffsetsWorkspace");
    maskWS = alg->getProperty("OutputMaskWorkspace");
    AnalysisDataService::Instance().addOrReplace(instName+"_group", groupWS);
    AnalysisDataService::Instance().addOrReplace(instName+"_offsets", offsetsWS);
    AnalysisDataService::Instance().addOrReplace(instName+"_mask", maskWS);
  }

  if (filterBadPulses)
  {
    API::IAlgorithm_sptr filterAlg = createSubAlgorithm("FilterBadPulses");
    filterAlg->setProperty("InputWorkspace", m_eventW);
    filterAlg->executeAsSubAlg();
    m_eventW = filterAlg->getProperty("OutputWorkspace");
  }

  if (removePromptPulseWidth > 0.)
  {
    API::IAlgorithm_sptr filterAlg = createSubAlgorithm("RemovePromptPulse");
    filterAlg->setProperty("InputWorkspace", m_eventW);
    filterAlg->setProperty("Width", removePromptPulseWidth);
    filterAlg->executeAsSubAlg();
    m_eventW = filterAlg->getProperty("OutputWorkspace");
  }

  if (!filterName.empty())
  {
    API::IAlgorithm_sptr filterLogsAlg = createSubAlgorithm("FilterByLogValue");
    filterLogsAlg->setProperty("InputWorkspace", m_eventW);
    filterLogsAlg->setProperty("LogName", filterName);
    filterLogsAlg->setProperty("MinimumValue", filterMin);
    filterLogsAlg->setProperty("MaximumValue", filterMax);
    filterLogsAlg->executeAsSubAlg();
    m_eventW = filterLogsAlg->getProperty("OutputWorkspace");
  }

  API::IAlgorithm_sptr compressAlg = createSubAlgorithm("CompressEvents");
  compressAlg->setProperty("InputWorkspace", m_eventW);
  compressAlg->setProperty("Tolerance",tolerance);
  compressAlg->executeAsSubAlg();
  m_eventW = compressAlg->getProperty("OutputWorkspace");

  m_eventW->sortAll(TOF_SORT, NULL);

  if (xmin > 0. || xmax > 0.)
  {
	  API::IAlgorithm_sptr cropAlg = createSubAlgorithm("CropWorkspace");
	  cropAlg->setProperty("InputWorkspace", m_inputW);
	  if (xmin > 0.)cropAlg->setProperty("Xmin", xmin);
	  if (xmax > 0.)cropAlg->setProperty("Xmax", xmax);
	  cropAlg->executeAsSubAlg();
	  m_inputW = cropAlg->getProperty("OutputWorkspace");
  }

  API::IAlgorithm_sptr maskAlg = createSubAlgorithm("MaskDetectors");
  maskAlg->setProperty("Workspace", m_inputW);
  maskAlg->setProperty("MaskedWorkspace", instName+"_mask");
  maskAlg->executeAsSubAlg();
  m_outputW = maskAlg->getProperty("Workspace");

  if(!dspace)
  {
	  API::IAlgorithm_sptr rebinAlg = createSubAlgorithm("Rebin");
	  rebinAlg->setProperty("InputWorkspace", m_outputW);
	  rebinAlg->setProperty("Params",params);
	  rebinAlg->executeAsSubAlg();
	  m_outputW = rebinAlg->getProperty("OutputWorkspace");
	  setProperty("OutputWorkspace",m_outputW);
  }

  API::IAlgorithm_sptr alignAlg = createSubAlgorithm("AlignDetectors");
  alignAlg->setProperty("InputWorkspace", m_outputW);
  alignAlg->setProperty("OffsetsWorkspace", instName+"_offsets");
  alignAlg->executeAsSubAlg();
  m_outputW = alignAlg->getProperty("OutputWorkspace");

  if(LRef > 0. || minwl > 0. || DIFCref > 0.)
  {
          API::IAlgorithm_sptr convertAlg = createSubAlgorithm("ConvertUnits");
          convertAlg->setProperty("InputWorkspace", m_outputW);
          convertAlg->setProperty("Target","TOF");
          convertAlg->executeAsSubAlg();
          m_outputW = convertAlg->getProperty("OutputWorkspace");
  }

  if(LRef > 0.)
  {
	  API::IAlgorithm_sptr removeAlg = createSubAlgorithm("RemoveLowResTOF");
	  removeAlg->setProperty("InputWorkspace", m_outputW);
	  removeAlg->setProperty("LRef",LRef);
	  removeAlg->setProperty("Tmin",tmin);
	  removeAlg->setProperty("Tmax",tmax);
	  removeAlg->executeAsSubAlg();
	  m_outputW = removeAlg->getProperty("OutputWorkspace");
	  setProperty("OutputWorkspace",m_outputW);
  }

  if(minwl > 0.)
  {
	  API::IAlgorithm_sptr removeAlg = createSubAlgorithm("RemoveLowResTOF");
	  removeAlg->setProperty("InputWorkspace", m_outputW);
	  removeAlg->setProperty("MinWavelength",minwl);
	  removeAlg->setProperty("Tmin",tmin);
	  removeAlg->executeAsSubAlg();
	  m_outputW = removeAlg->getProperty("OutputWorkspace");
	  setProperty("OutputWorkspace",m_outputW);
  }
  else if(DIFCref > 0.)
  {
	  API::IAlgorithm_sptr removeAlg = createSubAlgorithm("RemoveLowResTOF");
	  removeAlg->setProperty("InputWorkspace", m_outputW);
	  removeAlg->setProperty("ReferenceDIFC",DIFCref);
	  removeAlg->setProperty("K",3.22);
	  removeAlg->setProperty("Tmin",tmin);
	  removeAlg->executeAsSubAlg();
	  m_outputW = removeAlg->getProperty("OutputWorkspace");
	  setProperty("OutputWorkspace",m_outputW);
  }

  if(LRef > 0. || minwl > 0. || DIFCref > 0.)
  {
          API::IAlgorithm_sptr convertAlg = createSubAlgorithm("ConvertUnits");
          convertAlg->setProperty("InputWorkspace", m_outputW);
          convertAlg->setProperty("Target","dSpacing");
          convertAlg->executeAsSubAlg();
          m_outputW = convertAlg->getProperty("OutputWorkspace");
  }

  if(dspace)
  {
	  API::IAlgorithm_sptr rebinAlg = createSubAlgorithm("Rebin");
	  rebinAlg->setProperty("InputWorkspace", m_outputW);
	  rebinAlg->setProperty("Params",params);
	  rebinAlg->executeAsSubAlg();
	  m_outputW = rebinAlg->getProperty("OutputWorkspace");
	  setProperty("OutputWorkspace",m_outputW);
  }

  m_eventW->sortAll(TOF_SORT, NULL);

  API::IAlgorithm_sptr focusAlg = createSubAlgorithm("DiffractionFocussing");
  focusAlg->setProperty("InputWorkspace", m_outputW);
  focusAlg->setProperty("GroupingWorkspace", instName+"_group");
  focusAlg->setProperty("PreserveEvents", true);
  focusAlg->executeAsSubAlg();
  m_outputW = focusAlg->getProperty("OutputWorkspace");

  m_eventW->sortAll(TOF_SORT, NULL);

  API::IAlgorithm_sptr convertAlg = createSubAlgorithm("ConvertUnits");
  convertAlg->setProperty("InputWorkspace", m_outputW);
  convertAlg->setProperty("Target","TOF");
  convertAlg->executeAsSubAlg();
  m_outputW = convertAlg->getProperty("OutputWorkspace");

  API::IAlgorithm_sptr rebinAlg = createSubAlgorithm("Rebin");
  rebinAlg->setProperty("InputWorkspace", m_outputW);
  rebinAlg->setProperty("Params",params);
  rebinAlg->executeAsSubAlg();
  m_outputW = rebinAlg->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace",m_outputW);

}

} // namespace WorkflowAlgorithm
} // namespace Mantid
