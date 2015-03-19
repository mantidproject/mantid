#include "MantidMDAlgorithms/ConvertToMD.h"

#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/ProgressText.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/IPropertySettings.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/VisibleWhenProperty.h"
//
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidMDEvents/MDWSTransform.h"
//
#include "MantidDataObjects/Workspace2D.h"

#include <algorithm>
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidMDEvents/ConvToMDSelector.h"
#include "MantidDataObjects/TableWorkspace.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::MDEvents;
using namespace Mantid::MDEvents::CnvrtToMD;

namespace Mantid {
namespace MDAlgorithms {

//
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertToMD)
void ConvertToMD::init() {
  ConvertToMDParent::init();
  declareProperty(new WorkspaceProperty<IMDEventWorkspace>(
                      "OutputWorkspace", "", Direction::Output),
                  "Name of the output *MDEventWorkspace*.");

  declareProperty(
      new PropertyWithValue<bool>("OverwriteExisting", true, Direction::Input),
      "By default  (\"1\"), existing Output Workspace will be replaced. Select "
      "false (\"0\") if you want to add new events to the workspace, which "
      "already exist. "
      "\nChoosing \"0\" can be very inefficient for file-based workspaces");

  declareProperty(new ArrayProperty<double>("MinValues"),
                  "It has to be N comma separated values, where N is the "
                  "number of dimensions of the target workspace. Values "
                  "smaller then specified here will not be added to "
                  "workspace.\n Number N is defined by properties 4,6 and 7 "
                  "and "
                  "described on *MD Transformation factory* page. See also "
                  ":ref:`algm-ConvertToMDMinMaxLocal`");

  // TODO:    " If a minimal target workspace range is higher then the one
  // specified here, the target workspace range will be used instead " );

  declareProperty(new ArrayProperty<double>("MaxValues"),
                  "A list of the same size and the same units as MinValues "
                  "list. Values higher or equal to the specified by "
                  "this list will be ignored");
  // TODO:    "If a maximal target workspace range is lower, then one of
  // specified here, the target workspace range will be used instead" );

  // Box controller properties. These are the defaults
  this->initBoxControllerProps("5" /*SplitInto*/, 1000 /*SplitThreshold*/,
                               20 /*MaxRecursionDepth*/);
  // additional box controller settings property.
  auto mustBeMoreThen1 = boost::make_shared<BoundedValidator<int>>();
  mustBeMoreThen1->setLower(1);

  declareProperty(
      new PropertyWithValue<int>("MinRecursionDepth", 1, mustBeMoreThen1),
      "Optional. If specified, then all the boxes will be split to this "
      "minimum recursion depth. 0 = no splitting, "
      "1 = one level of splitting, etc. \n Be careful using this since it can "
      "quickly create a huge number of boxes = "
      "(SplitInto ^ (MinRercursionDepth * NumDimensions)). \n But setting this "
      "property equal to MaxRecursionDepth "
      "property is necessary if one wants to generate multiple file based "
      "workspaces in order to merge them later.");
  setPropertyGroup("MinRecursionDepth", getBoxSettingsGroupName());

  declareProperty(
       new PropertyWithValue<bool>("InitialSplitting", 0, Direction::Input),
       "This option causes an initial split of 50 for the first four dimensions at level 0.");
}
//----------------------------------------------------------------------------------------------
/** Destructor
*/
ConvertToMD::~ConvertToMD() {}

const std::string ConvertToMD::name() const { return "ConvertToMD"; }

int ConvertToMD::version() const { return 1; }

std::map<std::string, std::string> ConvertToMD::validateInputs() {
  std::map<std::string, std::string> result;

  std::vector<double> minVals = this->getProperty("MinValues");
  std::vector<double> maxVals = this->getProperty("MaxValues");

  if (minVals.size() != maxVals.size()) {
    std::stringstream msg;
    msg << "Rank of MinValues != MaxValues (" << minVals.size()
        << "!=" << maxVals.size() << ")";
    result["MinValues"] = msg.str();
    result["MaxValues"] = msg.str();
  } else {
    std::stringstream msg;

    size_t rank = minVals.size();
    for (size_t i = 0; i < rank; ++i) {
      if (minVals[i] >= maxVals[i]) {
        if (msg.str().empty())
          msg << "max not bigger than min ";
        else
          msg << ", ";
        msg << "at index=" << (i + 1) << " (" << minVals[i]
            << ">=" << maxVals[i] << ")";
      }
    }

    if (!msg.str().empty()) {
      result["MinValues"] = msg.str();
      result["MaxValues"] = msg.str();
    }
  }

  return result;
}

//----------------------------------------------------------------------------------------------
/* Execute the algorithm.   */
void ConvertToMD::exec() {
  // initiate class which would deal with any dimension workspaces requested by
  // algorithm parameters
  if (!m_OutWSWrapper)
    m_OutWSWrapper = boost::shared_ptr<MDEvents::MDEventWSWrapper>(
        new MDEvents::MDEventWSWrapper());

  // -------- get Input workspace
  m_InWS2D = getProperty("InputWorkspace");

  // get the output workspace
  API::IMDEventWorkspace_sptr spws = getProperty("OutputWorkspace");

  // Collect and Analyze the requests to the job, specified by the input
  // parameters:
  // a) Q selector:
  std::string QModReq = getProperty("QDimensions");
  // b) the energy exchange mode
  std::string dEModReq = getProperty("dEAnalysisMode");
  // c) other dim property;
  std::vector<std::string> otherDimNames = getProperty("OtherDimensions");
  // d) The output dimensions in the Q3D mode, processed together with
  // QConversionScales
  std::string QFrame = getProperty("Q3DFrames");
  // e) part of the procedure, specifying the target dimensions units. Currently
  // only Q3D target units can be converted to different flavors of hkl
  std::string convertTo_ = getProperty("QConversionScales");

  // get the min and max values for the dimensions from the input properties
  std::vector<double> dimMin = getProperty("MinValues");
  std::vector<double> dimMax = getProperty("MaxValues");

  // Build the target ws description as function of the input & output ws and
  // the parameters, supplied to the algorithm
  MDEvents::MDWSDescription targWSDescr;
  // get workspace parameters and build target workspace description, report if
  // there is need to build new target MD workspace
  bool createNewTargetWs =
      buildTargetWSDescription(spws, QModReq, dEModReq, otherDimNames, dimMin,
                               dimMax, QFrame, convertTo_, targWSDescr);

  // create and initiate new workspace or set up existing workspace as a target.
  if (createNewTargetWs) // create new
    spws = this->createNewMDWorkspace(targWSDescr);
  else // setup existing MD workspace as workspace target.
    m_OutWSWrapper->setMDWS(spws);

  // pre-process detectors;
  targWSDescr.m_PreprDetTable = this->preprocessDetectorsPositions(
      m_InWS2D, dEModReq, getProperty("UpdateMasks"),
      std::string(getProperty("PreprocDetectorsWS")));

  /// copy & retrieve metadata, necessary to initialize convertToMD Plugin,
  /// including getting the unique number, that identifies the run, the source
  /// workspace came from.
  addExperimentInfo(spws, targWSDescr);
  // get pointer to appropriate  ConverttToMD plugin from the CovertToMD plugins
  // factory, (will throw if logic is wrong and ChildAlgorithm is not found
  // among existing)
  ConvToMDSelector AlgoSelector;
  this->m_Convertor = AlgoSelector.convSelector(m_InWS2D, this->m_Convertor);

  bool ignoreZeros = getProperty("IgnoreZeroSignals");
  // initiate conversion and estimate amount of job to do
  size_t n_steps =
      this->m_Convertor->initialize(targWSDescr, m_OutWSWrapper, ignoreZeros);
  // copy the metadata, necessary for resolution corrections
  copyMetaData(spws);

  // progress reporter
  m_Progress.reset(new API::Progress(this, 0.0, 1.0, n_steps));

  g_log.information() << " conversion started\n";
  // DO THE JOB:
  this->m_Convertor->runConversion(m_Progress.get());

  // JOB COMPLETED:
  setProperty("OutputWorkspace",
              boost::dynamic_pointer_cast<IMDEventWorkspace>(spws));
  // free the algorithm from the responsibility for the target workspace to
  // allow it to be deleted if necessary
  m_OutWSWrapper->releaseWorkspace();
  // free up the sp to the input workspace, which would be deleted if nobody
  // needs it any more;
  m_InWS2D.reset();
  return;
}
/**
 * Copy over the part of metadata necessary to initialize ConvertToMD plugin
 *from the input matrix workspace to output MDEventWorkspace
 * @param mdEventWS :: The output MDEventWorkspace
 * @param targWSDescr :: The description of the target workspace, used in the
 *algorithm
 *
 * @return  :: modified targWSDescription containing the number of experiment
 *info added from the current MD workspace
 */
void
ConvertToMD::addExperimentInfo(API::IMDEventWorkspace_sptr &mdEventWS,
                               MDEvents::MDWSDescription &targWSDescr) const {
  // Copy ExperimentInfo (instrument, run, sample) to the output WS
  API::ExperimentInfo_sptr ei(m_InWS2D->cloneExperimentInfo());

  ei->mutableRun().addProperty("RUBW_MATRIX", targWSDescr.m_Wtransf.getVector(),
                               true);
  ei->mutableRun().addProperty(
      "W_MATRIX",
      targWSDescr.getPropertyValueAsType<std::vector<double>>("W_MATRIX"),
      true);

  // run index as the number of experiment into merged within this run. It is
  // possible to interpret it differently
  // and should never expect it to start with 0 (for first experiment info)
  uint16_t runIndex = mdEventWS->addExperimentInfo(ei);

  // add run-index to the target workspace description for further usage as the
  // identifier for the events, which come from this run.
  targWSDescr.addProperty("RUN_INDEX", runIndex, true);
}

/**
* Copy over the metadata from the input matrix workspace to output
*MDEventWorkspace
* @param mdEventWS :: The output MDEventWorkspace where metadata are copied to.
*The source of the metadata is the input matrix workspace
*
*/
void ConvertToMD::copyMetaData(API::IMDEventWorkspace_sptr &mdEventWS) const {

  // found detector which is not a monitor to get proper bin boundaries.
  size_t spectra_index(0);
  bool dector_found(false);
  for (size_t i = 0; i < m_InWS2D->getNumberHistograms(); ++i) {
    try {
      auto det = m_InWS2D->getDetector(i);
      if (!det->isMonitor()) {
        spectra_index = i;
        dector_found = true;
        g_log.debug() << "Using spectra N " << i << " as the source of the bin "
                                                    "boundaries for the "
                                                    "resolution corrections \n";
        break;
      }
    } catch (...) {
    }
  }
  if (!dector_found)
    g_log.warning() << "No detectors in the workspace are associated with "
                       "spectra. Using spectrum 0 trying to retrieve the bin "
                       "boundaries \n";

  // retrieve representative bin boundaries
  MantidVec binBoundaries = m_InWS2D->readX(spectra_index);
  // check if the boundaries transformation is necessary
  if (m_Convertor->getUnitConversionHelper().isUnitConverted()) {

    if (!dynamic_cast<DataObjects::EventWorkspace *>(m_InWS2D.get())) {
      g_log.information() << " ConvertToMD converts input workspace units, but "
                             "the bin boundaries are copied from the first "
                             "workspace spectra. The resolution estimates can "
                             "be incorrect if unit conversion depends on "
                             "spectra number.\n";

      UnitsConversionHelper &unitConv = m_Convertor->getUnitConversionHelper();
      unitConv.updateConversion(spectra_index);
      for (size_t i = 0; i < binBoundaries.size(); i++) {
        binBoundaries[i] = unitConv.convertUnits(binBoundaries[i]);
      }
    }
    // sort bin boundaries in case if unit transformation have swapped them.
    if (binBoundaries[0] > binBoundaries[binBoundaries.size() - 1]) {
      g_log.information() << "Bin boundaries are not arranged monotonously. "
                             "Sorting performed\n";
      std::sort(binBoundaries.begin(), binBoundaries.end());
    }
  }

  // Replacement for SpectraDetectorMap::createIDGroupsMap using the ISpectrum
  // objects instead
  auto mapping = boost::make_shared<det2group_map>();
  for (size_t i = 0; i < m_InWS2D->getNumberHistograms(); ++i) {
    const auto &dets = m_InWS2D->getSpectrum(i)->getDetectorIDs();
    if (!dets.empty()) {
      std::vector<detid_t> id_vector;
      std::copy(dets.begin(), dets.end(), std::back_inserter(id_vector));
      mapping->insert(std::make_pair(id_vector.front(), id_vector));
    }
  }

  uint16_t nexpts = mdEventWS->getNumExperimentInfo();
  for (uint16_t i = 0; i < nexpts; ++i) {
    ExperimentInfo_sptr expt = mdEventWS->getExperimentInfo(i);
    expt->mutableRun().storeHistogramBinBoundaries(binBoundaries);
    expt->cacheDetectorGroupings(*mapping);
  }
}

/** Constructor */
ConvertToMD::ConvertToMD() {}
/** handle the input parameters and build target workspace description as
function of input parameters
* @param spws shared pointer to target MD workspace (just created or already
existing)
* @param QModReq -- mode to convert momentum
* @param dEModReq -- mode to convert energy
* @param otherDimNames -- the vector of additional dimensions names (if any)
* @param dimMin     -- the vector of minimal values for all dimensions of the
workspace; on input it is copied from the algorithm parameters, on output
it is defined from MD workspace of matrix workspace depending on how well input
parameters are defined
* @param dimMax     -- the vector of maximal values for all dimensions of the
workspace; is set up similarly to dimMin
* @param QFrame      -- in Q3D case this describes target coordinate system and
is ignored in any other caste
* @param convertTo_  -- The parameter describing Q-scaling transformations
* @param targWSDescr -- the resulting class used to interpret all parameters
together and used to describe selected transformation.
*/
bool ConvertToMD::buildTargetWSDescription(
    API::IMDEventWorkspace_sptr spws, const std::string &QModReq,
    const std::string &dEModReq, const std::vector<std::string> &otherDimNames,
    std::vector<double> &dimMin, std::vector<double> &dimMax,
    const std::string &QFrame, const std::string &convertTo_,
    MDEvents::MDWSDescription &targWSDescr) {
  // ------- Is there need to create new output workspace?
  bool createNewTargetWs = doWeNeedNewTargetWorkspace(spws);
  std::vector<int> split_into;

  if (createNewTargetWs) {
    targWSDescr.m_buildingNewWorkspace = true;
    // find min-max dimensions values -- either take them from input parameters
    // or identify the defaults if input parameters are not defined
    this->findMinMax(m_InWS2D, QModReq, dEModReq, QFrame, convertTo_,
                     otherDimNames, dimMin, dimMax);
    // set number of bins each dimension split into.
    split_into = this->getProperty("SplitInto");
  } else // get min/max from existing MD workspace ignoring input min/max values
  {
    targWSDescr.m_buildingNewWorkspace = false;
    size_t NDims = spws->getNumDims();
    dimMin.resize(NDims);
    dimMax.resize(NDims);
    split_into.resize(NDims);
    for (size_t i = 0; i < NDims; i++) {
      const Geometry::IMDDimension *pDim = spws->getDimension(i).get();
      dimMin[i] = pDim->getMinimum();
      dimMax[i] = pDim->getMaximum();
      // number of dimension
      split_into[i] = static_cast<int>(pDim->getNBins());
    }
  }

  // verify that the number min/max values is equivalent to the number of
  // dimensions defined by properties and min is less max
  targWSDescr.setMinMax(dimMin, dimMax);
  targWSDescr.buildFromMatrixWS(m_InWS2D, QModReq, dEModReq, otherDimNames);
  targWSDescr.setNumBins(split_into);

  bool LorentzCorrections = getProperty("LorentzCorrection");
  targWSDescr.setLorentsCorr(LorentzCorrections);

  // instantiate class, responsible for defining Mslice-type projection
  MDEvents::MDWSTransform MsliceProj;
  // identify if u,v are present among input parameters and use defaults if not
  std::vector<double> ut = getProperty("UProj");
  std::vector<double> vt = getProperty("VProj");
  std::vector<double> wt = getProperty("WProj");
  try {
    // otherwise input uv are ignored -> later it can be modified to set ub
    // matrix if no given, but this may over-complicate things.
    MsliceProj.setUVvectors(ut, vt, wt);
  } catch (std::invalid_argument &) {
    g_log.error() << "The projections are coplanar. Will use defaults "
                     "[1,0,0],[0,1,0] and [0,0,1]" << std::endl;
  }

  if (createNewTargetWs) {

    // check if we are working in powder mode
    // set up target coordinate system and identify/set the (multi) dimension's
    // names to use
    targWSDescr.m_RotMatrix =
        MsliceProj.getTransfMatrix(targWSDescr, QFrame, convertTo_);
  } else // user input is mainly ignored and everything is in old MD workspace
  {
    // dimensions are already build, so build MDWS description from existing
    // workspace
    MDEvents::MDWSDescription oldWSDescr;
    oldWSDescr.buildFromMDWS(spws);

    // some conversion parameters can not be defined by the target workspace.
    // They have to be retrieved from the input workspace
    // and derived from input parameters.
    oldWSDescr.setUpMissingParameters(targWSDescr);
    // set up target coordinate system and the dimension names/units
    oldWSDescr.m_RotMatrix =
        MsliceProj.getTransfMatrix(oldWSDescr, QFrame, convertTo_);

    // check inconsistencies, if the existing workspace can be used as target
    // workspace.
    oldWSDescr.checkWSCorresponsMDWorkspace(targWSDescr);
    // reset new ws description name
    targWSDescr = oldWSDescr;
  }
  return createNewTargetWs;
}

/**
* Create new MD workspace and set up its box controller using algorithm's box
* controllers properties
* @param targWSDescr
* @return
*/
API::IMDEventWorkspace_sptr ConvertToMD::createNewMDWorkspace(
    const MDEvents::MDWSDescription &targWSDescr) {
  // create new md workspace and set internal shared pointer of m_OutWSWrapper
  // to this workspace
  API::IMDEventWorkspace_sptr spws =
      m_OutWSWrapper->createEmptyMDWS(targWSDescr);
  if (!spws) {
    g_log.error() << "can not create target event workspace with :"
                  << targWSDescr.nDimensions() << " dimensions\n";
    throw(std::invalid_argument("can not create target workspace"));
  }
  // Build up the box controller
  Mantid::API::BoxController_sptr bc =
      m_OutWSWrapper->pWorkspace()->getBoxController();
  // Build up the box controller, using the properties in
  // BoxControllerSettingsAlgorithm
  this->setBoxController(bc, m_InWS2D->getInstrument());

  // Check if the user want sto force an initial split or not
  bool initialSplittingChecked = this->getProperty("InitialSplitting");

  if (!initialSplittingChecked)
  {
    // split boxes;
    spws->splitBox();
  }
  else
  {
    // Perform initial split with the forced settings
    performInitialSplitting(spws, bc);
  }

  // Do we split more due to MinRecursionDepth?
  int minDepth = this->getProperty("MinRecursionDepth");
  int maxDepth = this->getProperty("MaxRecursionDepth");
  if (minDepth > maxDepth)
    throw std::invalid_argument(
        "MinRecursionDepth must be >= MaxRecursionDepth ");
  spws->setMinRecursionDepth(size_t(minDepth));

  return spws;
}

/**
 * Splits the initial box at level 0 into a defined number of subboxes for the the first level.
 * @param spws A pointer to the newly created event workspace.
 * @param bc A pointer to the box controller.
 */
void ConvertToMD::performInitialSplitting(API::IMDEventWorkspace_sptr spws, Mantid::API::BoxController_sptr bc)
{
  const size_t initialSplitSetting = 50;
  const size_t dimCutoff = 4;

  // Record the split settings of the box controller in a buffer and set the new value
  std::vector<size_t> splitBuffer;
  
  for (size_t dim = 0; dim < bc->getNDims(); dim++)
  {
    splitBuffer.push_back(bc->getSplitInto(dim));

    // Replace the box controller setting only for a max of the first three dimensions
    if (dim < dimCutoff)
    {
       bc->setSplitInto(dim, initialSplitSetting);
    }
  }

  // Perform the initial splitting
  spws->splitBox();

  // Revert changes on the box controller
  for (size_t dim = 0; dim < bc->getNDims(); ++dim)
  {
    bc->setSplitInto(dim, splitBuffer[dim]);
  }
}

/**Check if the target workspace new or exists and we need to create new
*workspace
*@param spws -- shared pointer to target MD workspace, which can be undefined if
*the workspace does not exist
*
*@returns true if one needs to create new workspace and false otherwise
*/
bool ConvertToMD::doWeNeedNewTargetWorkspace(API::IMDEventWorkspace_sptr spws) {

  bool createNewWs(false);
  if (!spws) {
    createNewWs = true;
  } else {
    bool shouldOverwrite = getProperty("OverwriteExisting");
    if (shouldOverwrite) {
      createNewWs = true;
    } else {
      createNewWs = false;
    }
  }
  return createNewWs;
}

/** Method takes min-max values from algorithm parameters if they are present or
*calculates default min-max values if these values
 *  were not supplied to the method or the supplied value is incorrect.
 *
*@param inWS     -- the shared pointer to the source workspace
*@param QMode    -- the string which defines algorithms Q-conversion mode
*@param dEMode   -- the string describes the algorithms energy conversion mode
*@param QFrame   -- in Q3D case this describes target coordinate system and is
*ignored in any other caste
*@param ConvertTo -- The parameter describing Q-scaling transformations
*@param otherDim -- the vector of other dimension names (if any)
*  Input-output values:
*@param minVal   -- the vector with min values for the algorithm
*@param maxVal   -- the vector with max values for the algorithm
*
*
*/
void ConvertToMD::findMinMax(
    const Mantid::API::MatrixWorkspace_sptr &inWS, const std::string &QMode,
    const std::string &dEMode, const std::string &QFrame,
    const std::string &ConvertTo, const std::vector<std::string> &otherDim,
    std::vector<double> &minVal, std::vector<double> &maxVal) {

  // get raw pointer to Q-transformation (do not delete this pointer, it hold by
  // MDTransfFatctory!)
  MDTransfInterface *pQtransf = MDTransfFactory::Instance().create(QMode).get();
  // get number of dimensions this Q transformation generates from the
  // workspace.
  auto iEmode = Kernel::DeltaEMode().fromString(dEMode);
  // get total number of dimensions the workspace would have.
  unsigned int nMatrixDim = pQtransf->getNMatrixDimensions(iEmode, inWS);
  // total number of dimensions
  size_t nDim = nMatrixDim + otherDim.size();

  // probably already have well defined min-max values, so no point of
  // pre-calculating them
  bool wellDefined(true);
  if ((nDim == minVal.size()) && (minVal.size() == maxVal.size())) {
    // are they indeed well defined?
    for (size_t i = 0; i < minVal.size(); i++) {
      if (minVal[i] >= maxVal[i]) // no it is ill defined
      {
        g_log.information()
            << " Min Value: " << minVal[i] << " for dimension N: " << i
            << " equal or exceeds max value:" << maxVal[i] << std::endl;
        wellDefined = false;
        break;
      }
    }
    if (wellDefined)
      return;
  }

  // we need to identify min-max values by themselves

  Mantid::API::Algorithm_sptr childAlg =
      createChildAlgorithm("ConvertToMDMinMaxLocal");
  if (!childAlg)
    throw(std::runtime_error(
        "Can not create child ChildAlgorithm to found min/max values"));

  childAlg->setPropertyValue("InputWorkspace", inWS->getName());
  childAlg->setPropertyValue("QDimensions", QMode);
  childAlg->setPropertyValue("dEAnalysisMode", dEMode);
  childAlg->setPropertyValue("Q3DFrames", QFrame);
  childAlg->setProperty("OtherDimensions", otherDim);
  childAlg->setProperty("QConversionScales", ConvertTo);
  childAlg->setProperty("PreprocDetectorsWS",
                        std::string(getProperty("PreprocDetectorsWS")));
  childAlg->execute();
  if (!childAlg->isExecuted())
    throw(std::runtime_error("Can not properly execute child algorithm to find "
                             "min/max workspace values"));

  minVal = childAlg->getProperty("MinValues");
  maxVal = childAlg->getProperty("MaxValues");

  // if some min-max values for dimensions produce ws with 0 width in this
  // direction, change it to have some width;
  for (unsigned int i = 0; i < nDim; i++) {
    if (minVal[i] >= maxVal[i]) {
      g_log.debug() << "identified min-max values for dimension N: " << i
                    << " are equal. Modifying min-max value to produce "
                       "dimension with 0.2*dimValue width\n";
      if (minVal[i] > 0) {
        minVal[i] *= 0.9;
        maxVal[i] *= 1.1;
      } else if (minVal[i] == 0) {
        minVal[i] = -0.1;
        maxVal[i] = 0.1;
      } else {
        minVal[i] *= 1.1;
        maxVal[i] *= 0.9;
      }
    } else // expand min-max values a bit to avoid cutting data on the edges
    {
      if (std::fabs(minVal[i]) > FLT_EPSILON)
        minVal[i] *= (1 + 2 * FLT_EPSILON);
      else
        minVal[i] -= 2 * FLT_EPSILON;
      if (std::fabs(minVal[i]) > FLT_EPSILON)
        maxVal[i] *= (1 + 2 * FLT_EPSILON);
      else
        minVal[i] += 2 * FLT_EPSILON;
    }
  }

  if (!wellDefined)
    return;

  // if only min or only max limits are defined and are well defined workspace,
  // the algorithm will use these limits
  std::vector<double> minAlgValues = this->getProperty("MinValues");
  std::vector<double> maxAlgValues = this->getProperty("MaxValues");
  bool allMinDefined = (minAlgValues.size() == nDim);
  bool allMaxDefined = (maxAlgValues.size() == nDim);
  if (allMinDefined || allMaxDefined) {
    for (size_t i = 0; i < nDim; i++) {
      if (allMinDefined)
        minVal[i] = minAlgValues[i];
      if (allMaxDefined)
        maxVal[i] = maxAlgValues[i];
    }
  }
}

} // namespace Mantid
} // namespace MDAlgorithms
