/*WIKI*

This is a workflow algorithm that does the bulk of the work for time focusing diffraction data. This is done by executing several sub-algorithms as listed below.

# [[RemovePromptPulse]] (event workspace only)
# [[CompressEvents]] (event workspace only)
# [[CropWorkspace]]
# [[MaskDetectors]]
# [[Rebin]] or [[ResampleX]] if not d-space binning
# [[AlignDetectors]]
# If LRef, minwl, or DIFCref are specified:
## [[ConvertUnits]] to time-of-flight
## [[UnwrapSNS]]
## [[RemoveLowResTOF]]
## [[ConvertUnits]] to d-spacing
# [[Rebin]] if d-space binning
# [[DiffractionFocussing]]
# [[SortEvents]] (event workspace only)
# [[EditInstrumentGeometry]] (if appropriate)
# [[ConvertUnits]] to time-of-f
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
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/System.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/FileFinder.h"

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
  declareProperty(new ArrayProperty<double>("Params"/*, boost::make_shared<RebinParamsValidator>()*/),
        "A comma separated list of first bin boundary, width, last bin boundary. Optionally\n"
        "this can be followed by a comma and more widths and last boundary pairs.\n"
        "Negative width values indicate logarithmic binning.");
  declareProperty("ResampleX", 0,
                  "Number of bins in x-axis. Non-zero value overrides \"Params\" property. Negative value means logorithmic binning.");
  setPropertySettings("Params", new EnabledWhenProperty("ResampleX", IS_DEFAULT));
  declareProperty("Dspacing", true,"Bin in Dspace. (True is Dspace; False is TOF)");
  declareProperty(new ArrayProperty<double>("DMin"), "Minimum for Dspace axis. (Default 0.) ");
  declareProperty(new ArrayProperty<double>("DMax"), "Maximum for Dspace axis. (Default 0.) ");
  declareProperty("TMin", 0.0, "Minimum for TOF axis. (Default 0.) ");
  declareProperty("TMax", 0.0, "Maximum for TOF or dspace axis. (Default 0.) ");
  declareProperty("PreserveEvents", true,
    "If the InputWorkspace is an EventWorkspace, this will preserve the full event list (warning: this will use much more memory!).");
  declareProperty("RemovePromptPulseWidth", 0.,
    "Width of events (in microseconds) near the prompt pulse to remove. 0 disables");
  declareProperty("CompressTolerance", 0.01,
    "Compress events (in microseconds) within this tolerance. (Default 0.01) ");
  declareProperty("UnwrapRef", 0., "Reference total flight path for frame unwrapping. Zero skips the correction");
  declareProperty("LowResRef", 0., "Reference DIFC for resolution removal. Zero skips the correction");
  declareProperty("CropWavelengthMin", 0., "Crop the data at this minimum wavelength. Overrides LowResRef.");
  declareProperty("PrimaryFlightPath", -1.0, "If positive, focus positions are changed.  (Default -1) ");
  declareProperty(new ArrayProperty<int32_t>("SpectrumIDs"),
    "Optional: Spectrum IDs (note that it is not detector ID or workspace indices).");
  declareProperty(new ArrayProperty<double>("L2"), "Optional: Secondary flight (L2) paths for each detector");
  declareProperty(new ArrayProperty<double>("Polar"), "Optional: Polar angles (two thetas) for detectors");
  declareProperty(new ArrayProperty<double>("Azimuthal"), "Azimuthal angles (out-of-plain) for detectors");

}

/** Executes the algorithm
 *
 *  @throw Exception::FileError If the grouping file cannot be opened or read successfully
 *  @throw runtime_error If unable to run one of the Child Algorithms successfully
 */
void AlignAndFocusPowder::exec()
{
  // retrieve the properties
  m_inputW = getProperty("InputWorkspace");
  m_inputEW = boost::dynamic_pointer_cast<EventWorkspace>( m_inputW );
  m_instName = m_inputW->getInstrument()->getName();
  m_instName = Kernel::ConfigService::Instance().getInstrument(m_instName).shortName();
  std::string calFileName = getPropertyValue("CalFileName");
  m_offsetsWS = getProperty("OffsetsWorkspace");
  m_maskWS = getProperty("MaskWorkspace");
  m_groupWS = getProperty("GroupingWorkspace");
  l1 = getProperty("PrimaryFlightPath");
  specids = getProperty("SpectrumIDs");
  l2s = getProperty("L2");
  tths = getProperty("Polar");
  phis = getProperty("Azimuthal");
  m_params=getProperty("Params");
  dspace = getProperty("DSpacing");
  m_dmins = getProperty("DMin");
  m_dmaxs = getProperty("DMax");
  double dmin = 0.;
  if (!m_dmins.empty())
    dmin = m_dmins[0];
  double dmax = 0.;
  if (!m_dmaxs.empty())
    dmax = m_dmaxs[0];
  LRef = getProperty("UnwrapRef");
  DIFCref = getProperty("LowResRef");
  minwl = getProperty("CropWavelengthMin");
  tmin = getProperty("TMin");
  tmax = getProperty("TMax");
  m_preserveEvents = getProperty("PreserveEvents");
  m_resampleX = getProperty("ResampleX");
  // determine some bits about d-space and binning
  if (m_resampleX != 0)
  {
    m_params.clear(); // ignore the normal rebin parameters
  }
  else if (m_params.size() == 1)
  {
    if (dmax > 0.)
      dspace = true;
    else
      dspace=false;
  }
  if (dspace)
  {
    if (m_params.size() == 1 && dmax > 0)
    {
        double step = m_params[0];
        m_params.clear();
        if (step > 0 || dmin > 0)
        {
          m_params.push_back(dmin);
          m_params.push_back(step);
          m_params.push_back(dmax);
          g_log.information() << "d-Spacing Binning: " << m_params[0] << "  " << m_params[1] << "  " << m_params[2] <<"\n";
        }
    }
  }
  else
  {
    if (m_params.size() == 1 && tmax > 0)
    {
        double step = m_params[0];
        if (step > 0 || tmin > 0)
        {
          m_params[0] = tmin;
          m_params.push_back(step);
          m_params.push_back(tmax);
          g_log.information() << "TOF Binning: " << m_params[0] << "  " << m_params[1] << "  " << m_params[2] <<"\n";
        }
    }
  }
  xmin = 0;
  xmax = 0;
  if (tmin > 0.)
  {
    xmin = tmin;
  }
  if (tmax > 0.)
  {
    xmax = tmax;
  }
  if (!dspace && m_params.size() == 3)
  {
    xmin = m_params[0];
    xmax = m_params[2];
  }

  loadCalFile(calFileName);


  // Now setup the output workspace
  m_outputW = getProperty("OutputWorkspace");
  if ( m_outputW == m_inputW )
  {
    if (m_inputEW)
    {
      m_outputEW = boost::dynamic_pointer_cast<EventWorkspace>(m_outputW);
    }
  }
  else
  {
    if (m_inputEW)
    {
      //Make a brand new EventWorkspace
      m_outputEW = boost::dynamic_pointer_cast<EventWorkspace>(
      WorkspaceFactory::Instance().create("EventWorkspace", m_inputEW->getNumberHistograms(), 2, 1));
      //Copy geometry over.
      WorkspaceFactory::Instance().initializeFromParent(m_inputEW, m_outputEW, false);
      //You need to copy over the data as well.
      m_outputEW->copyDataFrom( (*m_inputEW) );

      //Cast to the matrixOutputWS and save it
      m_outputW = boost::dynamic_pointer_cast<MatrixWorkspace>(m_outputEW);
      //m_outputW->setName(getProperty("OutputWorkspace"));
    }
    else
    {
      // Not-an-event workspace
      m_outputW = WorkspaceFactory::Instance().create(m_inputW);
      //m_outputW->setName(getProperty("OutputWorkspace"));
    }
  }

  // filter the input events if appropriate
  if (m_inputEW)
  {
    double removePromptPulseWidth = getProperty("RemovePromptPulseWidth");
    if (removePromptPulseWidth > 0.)
    {
      g_log.information() << "running RemovePromptPulse(Width="
                          << removePromptPulseWidth << ")\n";
      API::IAlgorithm_sptr filterPAlg = createChildAlgorithm("RemovePromptPulse");
      filterPAlg->setProperty("InputWorkspace", m_outputW);
      filterPAlg->setProperty("OutputWorkspace", m_outputW);
      filterPAlg->setProperty("Width", removePromptPulseWidth);
      filterPAlg->executeAsChildAlg();
      m_outputW = filterPAlg->getProperty("OutputWorkspace");
      m_outputEW = boost::dynamic_pointer_cast<EventWorkspace>(m_outputW);
    }

    double tolerance = getProperty("CompressTolerance");
    if (tolerance > 0.)
    {
      g_log.information() << "running CompressEvents(Tolerance=" << tolerance << ")\n";
      API::IAlgorithm_sptr compressAlg = createChildAlgorithm("CompressEvents");
      compressAlg->setProperty("InputWorkspace", m_outputEW);
      compressAlg->setProperty("OutputWorkspace", m_outputEW);
      compressAlg->setProperty("OutputWorkspace", m_outputEW);
      compressAlg->setProperty("Tolerance",tolerance);
      compressAlg->executeAsChildAlg();
      m_outputEW = compressAlg->getProperty("OutputWorkspace");
      m_outputW = boost::dynamic_pointer_cast<MatrixWorkspace>(m_outputEW);
    }
    else
    {
      g_log.information() << "Not compressing event list\n";
      doSortEvents(m_outputW); // still sort to help some thing out
    }
  }

  if (xmin > 0. || xmax > 0.)
  {
    bool doCorrection(true);
    if (m_outputEW) { // extra check for event workspaces
      doCorrection = (m_outputEW->getNumberEvents() > 0);
    }

    if (doCorrection) {
      g_log.information() << "running CropWorkspace(Xmin=" << xmin
                          << ", Xmax=" << xmax << ")\n" ;
	  API::IAlgorithm_sptr cropAlg = createChildAlgorithm("CropWorkspace");
	  cropAlg->setProperty("InputWorkspace", m_outputW);
	  cropAlg->setProperty("OutputWorkspace", m_outputW);
	  if (xmin > 0.)cropAlg->setProperty("Xmin", xmin);
	  if (xmax > 0.)cropAlg->setProperty("Xmax", xmax);
	  cropAlg->executeAsChildAlg();
	  m_outputW = cropAlg->getProperty("OutputWorkspace");
    }
  }

  g_log.information() << "running MaskDetectors\n";
  API::IAlgorithm_sptr maskAlg = createChildAlgorithm("MaskDetectors");
  maskAlg->setProperty("Workspace", m_outputW);
  maskAlg->setProperty("MaskedWorkspace", m_maskWS);
  maskAlg->executeAsChildAlg();
  m_outputW = maskAlg->getProperty("Workspace");

  if(!dspace)
    this->rebin();

  g_log.information() << "running AlignDetectors\n";
  API::IAlgorithm_sptr alignAlg = createChildAlgorithm("AlignDetectors");
  alignAlg->setProperty("InputWorkspace", m_outputW);
  alignAlg->setProperty("OutputWorkspace", m_outputW);
  alignAlg->setProperty("OffsetsWorkspace", m_offsetsWS);
  alignAlg->executeAsChildAlg();
  m_outputW = alignAlg->getProperty("OutputWorkspace");

  if(LRef > 0. || minwl > 0. || DIFCref > 0.)
  {
      g_log.information() << "running ConvertUnits(Target=TOF)\n";
	  API::IAlgorithm_sptr convert1Alg = createChildAlgorithm("ConvertUnits");
	  convert1Alg->setProperty("InputWorkspace", m_outputW);
	  convert1Alg->setProperty("OutputWorkspace", m_outputW);
	  convert1Alg->setProperty("Target","TOF");
	  convert1Alg->executeAsChildAlg();
	  m_outputW = convert1Alg->getProperty("OutputWorkspace");
  }

  if(LRef > 0.)
  {
      g_log.information() << "running UnwrapSNS(LRef=" << LRef
                          << ",Tmin=" << tmin << ",Tmax=" << tmax <<")\n";
	  API::IAlgorithm_sptr removeAlg = createChildAlgorithm("UnwrapSNS");
	  removeAlg->setProperty("InputWorkspace", m_outputW);
	  removeAlg->setProperty("OutputWorkspace", m_outputW);
	  removeAlg->setProperty("LRef",LRef);
	  if(tmin > 0.) removeAlg->setProperty("Tmin",tmin);
	  if(tmax > tmin) removeAlg->setProperty("Tmax",tmax);
	  removeAlg->executeAsChildAlg();
	  m_outputW = removeAlg->getProperty("OutputWorkspace");
  }

  if(minwl > 0.)
  {
      g_log.information() << "running RemoveLowResTOF(MinWavelength=" << minwl
                          << ",Tmin=" << tmin << ")\n";
	  API::IAlgorithm_sptr removeAlg = createChildAlgorithm("RemoveLowResTOF");
	  removeAlg->setProperty("InputWorkspace", m_outputW);
	  removeAlg->setProperty("OutputWorkspace", m_outputW);
	  removeAlg->setProperty("MinWavelength",minwl);
	  if(tmin > 0.) removeAlg->setProperty("Tmin",tmin);
	  removeAlg->executeAsChildAlg();
	  m_outputW = removeAlg->getProperty("OutputWorkspace");
  }
  else if(DIFCref > 0.)
  {
      g_log.information() << "running RemoveLowResTof(RefDIFC=" << DIFCref
                          << ",K=3.22)\n";
	  API::IAlgorithm_sptr removeAlg = createChildAlgorithm("RemoveLowResTOF");
	  removeAlg->setProperty("InputWorkspace", m_outputW);
	  removeAlg->setProperty("OutputWorkspace", m_outputW);
	  removeAlg->setProperty("ReferenceDIFC",DIFCref);
	  removeAlg->setProperty("K",3.22);
	  if(tmin > 0.) removeAlg->setProperty("Tmin",tmin);
	  removeAlg->executeAsChildAlg();
	  m_outputW = removeAlg->getProperty("OutputWorkspace");
  }

  if(LRef > 0. || minwl > 0. || DIFCref > 0.)
  {
      g_log.information() << "running ConvertUnits(Target=dSpacing)\n";
	  API::IAlgorithm_sptr convert2Alg = createChildAlgorithm("ConvertUnits");
	  convert2Alg->setProperty("InputWorkspace", m_outputW);
	  convert2Alg->setProperty("OutputWorkspace", m_outputW);
	  convert2Alg->setProperty("Target","dSpacing");
	  convert2Alg->executeAsChildAlg();
	  m_outputW = convert2Alg->getProperty("OutputWorkspace");
  }

  if(dspace)
    this->rebin();

  doSortEvents(m_outputW);

  g_log.information() << "running DiffractionFocussing\n";
  API::IAlgorithm_sptr focusAlg = createChildAlgorithm("DiffractionFocussing");
  focusAlg->setProperty("InputWorkspace", m_outputW);
  focusAlg->setProperty("OutputWorkspace", m_outputW);
  focusAlg->setProperty("GroupingWorkspace", m_groupWS);
  focusAlg->setProperty("PreserveEvents", m_preserveEvents);
  focusAlg->executeAsChildAlg();
  m_outputW = focusAlg->getProperty("OutputWorkspace");

  doSortEvents(m_outputW);

  // this next call should probably be in for rebin as well
  // but it changes the system tests
  if (dspace && m_resampleX != 0)
    this->rebin();

  if (l1 > 0)
  {
    g_log.information() << "running EditInstrumentGeometry\n";
    API::IAlgorithm_sptr editAlg = createChildAlgorithm("EditInstrumentGeometry");
    editAlg->setProperty("Workspace", m_outputW);
    editAlg->setProperty("NewInstrument", false);
    editAlg->setProperty("PrimaryFlightPath", l1);
    editAlg->setProperty("Polar", tths);
    editAlg->setProperty("SpectrumIDs", specids);
    editAlg->setProperty("L2", l2s);
    editAlg->setProperty("Azimuthal", phis);
    editAlg->executeAsChildAlg();
    m_outputW = editAlg->getProperty("Workspace");
  }

  g_log.information() << "running ConvertUnits\n";
  API::IAlgorithm_sptr convert3Alg = createChildAlgorithm("ConvertUnits");
  convert3Alg->setProperty("InputWorkspace", m_outputW);
  convert3Alg->setProperty("OutputWorkspace", m_outputW);
  convert3Alg->setProperty("Target","TOF");
  convert3Alg->executeAsChildAlg();
  m_outputW = convert3Alg->getProperty("OutputWorkspace");

  if ((!m_params.empty()) && (m_params.size() != 1))
  {
    m_params.erase(m_params.begin());
    m_params.pop_back();
  }
  if (!m_dmins.empty())
    m_dmins.clear();
  if (!m_dmaxs.empty())
    m_dmaxs.clear();

  this->rebin();

  // return the output workspace
  setProperty("OutputWorkspace",m_outputW);
}

void AlignAndFocusPowder::rebin()
{
  if (m_resampleX != 0)
  {
    g_log.information() << "running ResampleX(NumberBins=" << abs(m_resampleX)
                        << ", LogBinning=" << (m_resampleX < 0)
                        << ", dMin(" << m_dmins.size() << "), dmax(" << m_dmaxs.size() << "))\n";
    API::IAlgorithm_sptr alg = createChildAlgorithm("ResampleX");
    alg->setProperty("InputWorkspace", m_outputW);
    alg->setProperty("OutputWorkspace", m_outputW);
    if ((!m_dmins.empty()) && (!m_dmaxs.empty()))
    {
      size_t numHist = m_outputW->getNumberHistograms();
      if ((numHist == m_dmins.size()) && (numHist == m_dmaxs.size()))
      {
        alg->setProperty("XMin", m_dmins);
        alg->setProperty("XMax", m_dmaxs);
      }
      else
      {
        g_log.information() << "Number of dmin and dmax values don't match the "
                            << "number of workspace indices. Ignoring the parameters.\n";
      }
    }
    alg->setProperty("NumberBins", abs(m_resampleX));
    alg->setProperty("LogBinning", (m_resampleX < 0));
    alg->executeAsChildAlg();
    m_outputW = alg->getProperty("OutputWorkspace");
  }
  else
  {
    g_log.information() << "running Rebin( ";
    for (auto param = m_params.begin(); param != m_params.end(); ++param)
      g_log.information() << (*param) << " ";
    g_log.information() << ")\n";
    API::IAlgorithm_sptr rebin3Alg = createChildAlgorithm("Rebin");
    rebin3Alg->setProperty("InputWorkspace", m_outputW);
    rebin3Alg->setProperty("OutputWorkspace", m_outputW);
    rebin3Alg->setProperty("Params",m_params);
    rebin3Alg->executeAsChildAlg();
    m_outputW = rebin3Alg->getProperty("OutputWorkspace");
  }
}

/**
 * Loads the .cal file if necessary.
 */
void AlignAndFocusPowder::loadCalFile(const std::string &calFileName)
{

  // check if the workspaces exist with their canonical names so they are not reloaded for chunks
  if (!m_groupWS)
  {
    try {
      m_groupWS = AnalysisDataService::Instance().retrieveWS<GroupingWorkspace>(m_instName+"_group");
    } catch (Exception::NotFoundError&) {
      ; // not noteworthy
    }
  }
  if (!m_offsetsWS)
  {
    try {
      m_offsetsWS = AnalysisDataService::Instance().retrieveWS<OffsetsWorkspace>(m_instName+"_offsets");
    } catch (Exception::NotFoundError&) {
      ; // not noteworthy
    }
  }
  if (!m_maskWS)
  {
    try {
      m_maskWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_instName+"_mask");
    } catch (Exception::NotFoundError&) {
      ; // not noteworthy
    }
  }

  // see if everything exists to exit early
  if (m_groupWS && m_offsetsWS && m_maskWS)
    return;

  g_log.information() << "Loading Calibration file \"" << calFileName << "\"\n";

  // bunch of booleans to keep track of things
  bool loadGrouping = !m_groupWS;
  bool loadOffsets  = !m_offsetsWS;
  bool loadMask     = !m_maskWS;

  // Load the .cal file
  IAlgorithm_sptr alg = createChildAlgorithm("LoadCalFile");
  alg->setPropertyValue("CalFilename", calFileName);
  alg->setProperty("InputWorkspace", m_inputW);
  alg->setProperty<std::string>("WorkspaceName", m_instName);
  alg->setProperty("MakeGroupingWorkspace", loadGrouping);
  alg->setProperty("MakeOffsetsWorkspace",  loadOffsets);
  alg->setProperty("MakeMaskWorkspace",     loadMask);
  alg->setLogging(true);
  alg->executeAsChildAlg();

  // replace workspaces as appropriate
  if (loadGrouping)
  {
    m_groupWS = alg->getProperty("OutputGroupingWorkspace");
    AnalysisDataService::Instance().addOrReplace(m_instName+"_group", m_groupWS);
  }
  if (loadOffsets)
  {
    m_offsetsWS = alg->getProperty("OutputOffsetsWorkspace");
    AnalysisDataService::Instance().addOrReplace(m_instName+"_offsets", m_offsetsWS);
  }
  if (loadMask)
  {
    m_maskWS = alg->getProperty("OutputMaskWorkspace");
    AnalysisDataService::Instance().addOrReplace(m_instName+"_mask", m_maskWS);
  }
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
    Algorithm_sptr alg = this->createChildAlgorithm("SortEvents");
    alg->setProperty("InputWorkspace", eventWS);
    alg->setPropertyValue("SortBy", "X Value");
    alg->executeAsChildAlg();
  }


} // namespace WorkflowAlgorithm
} // namespace Mantid
