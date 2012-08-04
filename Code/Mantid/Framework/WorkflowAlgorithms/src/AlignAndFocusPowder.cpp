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
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
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
  declareProperty(new ArrayProperty<double>("Params", boost::make_shared<RebinParamsValidator>()),
        "A comma separated list of first bin boundary, width, last bin boundary. Optionally\n"
        "this can be followed by a comma and more widths and last boundary pairs.\n"
        "Negative width values indicate logarithmic binning.");
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
  if ((m_eventW != NULL))
  {
    //Input workspace is an event workspace. Use the other exec method
    this->execEvent();
    return;
  }
  std::string instName = m_inputW->getInstrument()->getName();
  std::string calFileName=getProperty("CalFileName");
  OffsetsWorkspace_sptr offsetsWS = getProperty("OffsetsWorkspace");
  MatrixWorkspace_sptr maskWS = getProperty("MaskWorkspace");
  GroupingWorkspace_sptr groupWS = getProperty("GroupingWorkspace");
  std::vector<double> params=getProperty("Params");
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
  // Now create the output workspace
  m_outputW = getProperty("OutputWorkspace");
  if ( m_outputW != m_inputW )
  {
     m_outputW = WorkspaceFactory::Instance().create(m_inputW);
     m_outputW->setName(getProperty("OutputWorkspace"));
  }

  if (xmin > 0. || xmax > 0.)
  {
	  API::IAlgorithm_sptr cropAlg = createSubAlgorithm("CropWorkspace");
	  cropAlg->setProperty("InputWorkspace", m_outputW);
	  if (xmin > 0.)cropAlg->setProperty("Xmin", xmin);
	  if (xmax > 0.)cropAlg->setProperty("Xmax", xmax);
	  cropAlg->executeAsSubAlg();
	  m_outputW = cropAlg->getProperty("OutputWorkspace");
  }

  API::IAlgorithm_sptr maskAlg = createSubAlgorithm("MaskDetectors");
  maskAlg->setProperty("Workspace", m_outputW);
  maskAlg->setProperty("MaskedWorkspace", instName+"_mask");
  maskAlg->executeAsSubAlg();
  m_outputW = maskAlg->getProperty("Workspace");

  if(!dspace)
  {
	  API::IAlgorithm_sptr rebin1Alg = createSubAlgorithm("Rebin");
	  rebin1Alg->setProperty("InputWorkspace", m_outputW);
	  rebin1Alg->setProperty("Params",params);
	  rebin1Alg->executeAsSubAlg();
	  m_outputW = rebin1Alg->getProperty("OutputWorkspace");
  }

  API::IAlgorithm_sptr alignAlg = createSubAlgorithm("AlignDetectors");
  alignAlg->setProperty("InputWorkspace", m_outputW);
  alignAlg->setProperty("OffsetsWorkspace", instName+"_offsets");
  alignAlg->executeAsSubAlg();
  m_outputW = alignAlg->getProperty("OutputWorkspace");

  if(LRef > 0. || minwl > 0. || DIFCref > 0.)
  {
	  API::IAlgorithm_sptr convert1Alg = createSubAlgorithm("ConvertUnits");
	  convert1Alg->setProperty("InputWorkspace", m_outputW);
	  convert1Alg->setProperty("Target","TOF");
	  convert1Alg->executeAsSubAlg();
	  m_outputW = convert1Alg->getProperty("OutputWorkspace");
  }

  if(LRef > 0.)
  {
	  API::IAlgorithm_sptr removeAlg = createSubAlgorithm("UnwrapSNS");
	  removeAlg->setProperty("InputWorkspace", m_outputW);
	  removeAlg->setProperty("LRef",LRef);
	  if(tmin > 0.) removeAlg->setProperty("Tmin",tmin);
	  if(tmax > tmin) removeAlg->setProperty("Tmax",tmax);
	  removeAlg->executeAsSubAlg();
	  m_outputW = removeAlg->getProperty("OutputWorkspace");
  }

  if(minwl > 0.)
  {
	  API::IAlgorithm_sptr removeAlg = createSubAlgorithm("RemoveLowResTOF");
	  removeAlg->setProperty("InputWorkspace", m_outputW);
	  removeAlg->setProperty("MinWavelength",minwl);
	  if(tmin > 0.) removeAlg->setProperty("Tmin",tmin);
	  removeAlg->executeAsSubAlg();
	  m_outputW = removeAlg->getProperty("OutputWorkspace");
  }
  else if(DIFCref > 0.)
  {
	  API::IAlgorithm_sptr removeAlg = createSubAlgorithm("RemoveLowResTOF");
	  removeAlg->setProperty("InputWorkspace", m_outputW);
	  removeAlg->setProperty("ReferenceDIFC",DIFCref);
	  removeAlg->setProperty("K",3.22);
	  if(tmin > 0.) removeAlg->setProperty("Tmin",tmin);
	  removeAlg->executeAsSubAlg();
	  m_outputW = removeAlg->getProperty("OutputWorkspace");
  }

  if(LRef > 0. || minwl > 0. || DIFCref > 0.)
  {
	  API::IAlgorithm_sptr convert2Alg = createSubAlgorithm("ConvertUnits");
	  convert2Alg->setProperty("InputWorkspace", m_outputW);
	  convert2Alg->setProperty("Target","dSpacing");
	  convert2Alg->executeAsSubAlg();
	  m_outputW = convert2Alg->getProperty("OutputWorkspace");
  }

  if(dspace)
  {
	  API::IAlgorithm_sptr rebin2Alg = createSubAlgorithm("Rebin");
	  rebin2Alg->setProperty("InputWorkspace", m_outputW);
	  rebin2Alg->setProperty("Params",params);
	  rebin2Alg->executeAsSubAlg();
	  m_outputW = rebin2Alg->getProperty("OutputWorkspace");
  }

  API::IAlgorithm_sptr focusAlg = createSubAlgorithm("DiffractionFocussing");
  focusAlg->setProperty("InputWorkspace", m_outputW);
  focusAlg->setProperty("GroupingWorkspace", instName+"_group");
  focusAlg->setProperty("PreserveEvents", false);
  focusAlg->executeAsSubAlg();
  m_outputW = focusAlg->getProperty("OutputWorkspace");

  API::IAlgorithm_sptr convert3Alg = createSubAlgorithm("ConvertUnits");
  convert3Alg->setProperty("InputWorkspace", m_outputW);
  convert3Alg->setProperty("Target","TOF");
  convert3Alg->executeAsSubAlg();
  m_outputW = convert3Alg->getProperty("OutputWorkspace");

  if (params.size() != 1)
  {
  	params.erase(params.begin());
  	params.pop_back();
  }
  API::IAlgorithm_sptr rebin3Alg = createSubAlgorithm("Rebin");
  rebin3Alg->setProperty("InputWorkspace", m_outputW);
  rebin3Alg->setProperty("Params",params);
  rebin3Alg->executeAsSubAlg();
  m_outputW = rebin3Alg->getProperty("OutputWorkspace");
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
  std::vector<double> params=getProperty("Params");
  bool dspace = getProperty("DSpacing");
  double xmin = getProperty("CropMin");
  double xmax = getProperty("CropMax");
  double LRef = getProperty("UnwrapRef");
  double DIFCref = getProperty("LowResRef");
  double minwl = getProperty("CropWavelengthMin");
  double tmin = getProperty("TMin");
  double tmax = getProperty("TMax");
  bool preserveEvents = getProperty("PreserveEvents");
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
  Progress progress(this,0.0,1.0,m_eventW->getNumberHistograms());

  // generate the output workspace pointer
  m_outputW = getProperty("OutputWorkspace");
  EventWorkspace_sptr m_outputEventW;
  if (m_outputW == m_inputW)
  {
    m_outputEventW = boost::dynamic_pointer_cast<EventWorkspace>(m_outputW);
  }
  else
  {
    //Make a brand new EventWorkspace
    m_outputEventW = boost::dynamic_pointer_cast<EventWorkspace>(
    WorkspaceFactory::Instance().create("EventWorkspace", m_eventW->getNumberHistograms(), 2, 1));
    //Copy geometry over.
    WorkspaceFactory::Instance().initializeFromParent(m_eventW, m_outputEventW, false);
    //You need to copy over the data as well.
    m_outputEventW->copyDataFrom( (*m_eventW) );

    //Cast to the matrixOutputWS and save it
    m_outputW = boost::dynamic_pointer_cast<MatrixWorkspace>(m_outputEventW);
    m_outputW->setName(getProperty("OutputWorkspace"));
  }

  if (filterBadPulses)
  {
    API::IAlgorithm_sptr filterBAlg = createSubAlgorithm("FilterBadPulses");
    filterBAlg->setProperty("InputWorkspace", m_outputEventW);
    filterBAlg->setProperty("OutputWorkspace", m_outputEventW);
    filterBAlg->executeAsSubAlg();
    m_outputEventW = filterBAlg->getProperty("OutputWorkspace");
    m_outputW = boost::dynamic_pointer_cast<MatrixWorkspace>(m_outputEventW);
  }

  if (removePromptPulseWidth > 0.)
  {
    API::IAlgorithm_sptr filterPAlg = createSubAlgorithm("RemovePromptPulse");
    filterPAlg->setProperty("InputWorkspace", m_outputW);
    filterPAlg->setProperty("OutputWorkspace", m_outputW);
    filterPAlg->setProperty("Width", removePromptPulseWidth);
    filterPAlg->executeAsSubAlg();
    m_outputW = filterPAlg->getProperty("OutputWorkspace");
    m_outputEventW = boost::dynamic_pointer_cast<EventWorkspace>(m_outputW);
  }

  if (!filterName.empty())
  {
    API::IAlgorithm_sptr filterLogsAlg = createSubAlgorithm("FilterByLogValue");
    filterLogsAlg->setProperty("InputWorkspace", m_outputEventW);
    filterLogsAlg->setProperty("LogName", filterName);
    filterLogsAlg->setProperty("MinimumValue", filterMin);
    filterLogsAlg->setProperty("MaximumValue", filterMax);
    filterLogsAlg->executeAsSubAlg();
    m_outputEventW = filterLogsAlg->getProperty("OutputWorkspace");
    m_outputW = boost::dynamic_pointer_cast<MatrixWorkspace>(m_outputEventW);
  }

  API::IAlgorithm_sptr compressAlg = createSubAlgorithm("CompressEvents");
  compressAlg->setProperty("InputWorkspace", m_outputEventW);
  compressAlg->setProperty("OutputWorkspace", m_outputEventW);
  compressAlg->setProperty("Tolerance",tolerance);
  compressAlg->executeAsSubAlg();
  m_outputEventW = compressAlg->getProperty("OutputWorkspace");
  m_outputW = boost::dynamic_pointer_cast<MatrixWorkspace>(m_outputEventW);

  doSortEvents(m_outputW);

  if (xmin > 0. || xmax > 0.)
  {
	  API::IAlgorithm_sptr cropAlg = createSubAlgorithm("CropWorkspace");
	  cropAlg->setProperty("InputWorkspace", m_outputW);
	  if (xmin > 0.)cropAlg->setProperty("Xmin", xmin);
	  if (xmax > 0.)cropAlg->setProperty("Xmax", xmax);
	  cropAlg->executeAsSubAlg();
	  m_outputW = cropAlg->getProperty("OutputWorkspace");
  }

  API::IAlgorithm_sptr maskAlg = createSubAlgorithm("MaskDetectors");
  maskAlg->setProperty("Workspace", m_outputW);
  maskAlg->setProperty("MaskedWorkspace", instName+"_mask");
  maskAlg->executeAsSubAlg();
  m_outputW = maskAlg->getProperty("Workspace");

  if(!dspace)
  {
	  API::IAlgorithm_sptr rebin1Alg = createSubAlgorithm("Rebin");
	  rebin1Alg->setProperty("InputWorkspace", m_outputW);
	  rebin1Alg->setProperty("Params",params);
	  rebin1Alg->executeAsSubAlg();
	  m_outputW = rebin1Alg->getProperty("OutputWorkspace");
  }

  API::IAlgorithm_sptr alignAlg = createSubAlgorithm("AlignDetectors");
  alignAlg->setProperty("InputWorkspace", m_outputW);
  alignAlg->setProperty("OffsetsWorkspace", instName+"_offsets");
  alignAlg->executeAsSubAlg();
  m_outputW = alignAlg->getProperty("OutputWorkspace");

  if(LRef > 0. || minwl > 0. || DIFCref > 0.)
  {
	  API::IAlgorithm_sptr convert1Alg = createSubAlgorithm("ConvertUnits");
	  convert1Alg->setProperty("InputWorkspace", m_outputW);
	  convert1Alg->setProperty("Target","TOF");
	  convert1Alg->executeAsSubAlg();
	  m_outputW = convert1Alg->getProperty("OutputWorkspace");
  }

  if(LRef > 0.)
  {
	  API::IAlgorithm_sptr removeAlg = createSubAlgorithm("UnwrapSNS");
	  removeAlg->setProperty("InputWorkspace", m_outputW);
	  removeAlg->setProperty("LRef",LRef);
	  if(tmin > 0.) removeAlg->setProperty("Tmin",tmin);
	  if(tmax > tmin) removeAlg->setProperty("Tmax",tmax);
	  removeAlg->executeAsSubAlg();
	  m_outputW = removeAlg->getProperty("OutputWorkspace");
  }

  if(minwl > 0.)
  {
	  API::IAlgorithm_sptr removeAlg = createSubAlgorithm("RemoveLowResTOF");
	  removeAlg->setProperty("InputWorkspace", m_outputW);
	  removeAlg->setProperty("MinWavelength",minwl);
	  if(tmin > 0.) removeAlg->setProperty("Tmin",tmin);
	  removeAlg->executeAsSubAlg();
	  m_outputW = removeAlg->getProperty("OutputWorkspace");
  }
  else if(DIFCref > 0.)
  {
	  API::IAlgorithm_sptr removeAlg = createSubAlgorithm("RemoveLowResTOF");
	  removeAlg->setProperty("InputWorkspace", m_outputW);
	  removeAlg->setProperty("ReferenceDIFC",DIFCref);
	  removeAlg->setProperty("K",3.22);
	  if(tmin > 0.) removeAlg->setProperty("Tmin",tmin);
	  removeAlg->executeAsSubAlg();
	  m_outputW = removeAlg->getProperty("OutputWorkspace");
  }

  if(LRef > 0. || minwl > 0. || DIFCref > 0.)
  {
	  API::IAlgorithm_sptr convert2Alg = createSubAlgorithm("ConvertUnits");
	  convert2Alg->setProperty("InputWorkspace", m_outputW);
	  convert2Alg->setProperty("Target","dSpacing");
	  convert2Alg->executeAsSubAlg();
	  m_outputW = convert2Alg->getProperty("OutputWorkspace");
  }

  if(dspace)
  {
	  API::IAlgorithm_sptr rebin2Alg = createSubAlgorithm("Rebin");
	  rebin2Alg->setProperty("InputWorkspace", m_outputW);
	  rebin2Alg->setProperty("Params",params);
	  rebin2Alg->executeAsSubAlg();
	  m_outputW = rebin2Alg->getProperty("OutputWorkspace");
  }

  doSortEvents(m_outputW);

  API::IAlgorithm_sptr focusAlg = createSubAlgorithm("DiffractionFocussing");
  focusAlg->setProperty("InputWorkspace", m_outputW);
  focusAlg->setProperty("GroupingWorkspace", instName+"_group");
  focusAlg->setProperty("PreserveEvents", preserveEvents);
  focusAlg->executeAsSubAlg();
  m_outputW = focusAlg->getProperty("OutputWorkspace");

  doSortEvents(m_outputW);

  API::IAlgorithm_sptr convert3Alg = createSubAlgorithm("ConvertUnits");
  convert3Alg->setProperty("InputWorkspace", m_outputW);
  convert3Alg->setProperty("Target","TOF");
  convert3Alg->executeAsSubAlg();
  m_outputW = convert3Alg->getProperty("OutputWorkspace");

  if (params.size() != 1)
  {
  	params.erase(params.begin());
  	params.pop_back();
  }
  API::IAlgorithm_sptr rebin3Alg = createSubAlgorithm("Rebin");
  rebin3Alg->setProperty("InputWorkspace", m_outputW);
  rebin3Alg->setProperty("Params",params);
  rebin3Alg->executeAsSubAlg();
  m_outputW = rebin3Alg->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace",m_outputW);

}
  /** Perform SortEvents on the output workspaces 
   * but only if they are EventWorkspaces. 
   *
   * @param ws :: any Workspace. Does nothing if not EventWorkspace.
   */
  void AlignAndFocusPowder::doSortEvents(Mantid::API::Workspace_sptr ws)
  {
    EventWorkspace_sptr eventWS = boost::dynamic_pointer_cast<EventWorkspace>(ws);
    if (!eventWS)
      return;
    Algorithm_sptr alg = this->createSubAlgorithm("SortEvents");
    alg->setProperty("InputWorkspace", eventWS);
    alg->setPropertyValue("SortBy", "X Value");
    alg->executeAsSubAlg();
  }


} // namespace WorkflowAlgorithm
} // namespace Mantid
