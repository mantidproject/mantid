// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/ConvertToMD.h"

#include <algorithm>

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidKernel/EnabledWhenProperty.h"

#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/IPropertySettings.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/VisibleWhenProperty.h"

#include "MantidDataObjects/BoxControllerNeXusIO.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

#include "MantidGeometry/MDGeometry/MDHistoDimensionBuilder.h"

#include "MantidMDAlgorithms/ConvToMDEventsWSIndexing.h"
#include "MantidMDAlgorithms/ConvToMDSelector.h"
#include "MantidMDAlgorithms/MDTransfQ3D.h"
#include "MantidMDAlgorithms/MDWSTransform.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using Mantid::Geometry::MDHistoDimensionBuilder;

namespace Mantid {
namespace MDAlgorithms {

//
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertToMD)

void ConvertToMD::init() {
  ConvertToMDParent::init();
  declareProperty(make_unique<WorkspaceProperty<IMDEventWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Name of the output *MDEventWorkspace*.");

  declareProperty(
      make_unique<PropertyWithValue<bool>>("OverwriteExisting", true,
                                           Direction::Input),
      "By default  (\"1\"), existing Output Workspace will be replaced. Select "
      "false (\"0\") if you want to add new events to the workspace, which "
      "already exist. "
      "\nChoosing \"0\" can be very inefficient for file-based workspaces");

  declareProperty(make_unique<ArrayProperty<double>>("MinValues"),
                  "It has to be N comma separated values, where N is the "
                  "number of dimensions of the target workspace. Values "
                  "smaller then specified here will not be added to "
                  "workspace.\n Number N is defined by properties 4,6 and 7 "
                  "and "
                  "described on *MD Transformation factory* page. See also "
                  ":ref:`algm-ConvertToMDMinMaxLocal`");

  // TODO:    " If a minimal target workspace range is higher then the one
  // specified here, the target workspace range will be used instead " );

  declareProperty(make_unique<ArrayProperty<double>>("MaxValues"),
                  "A list of the same size and the same units as MinValues "
                  "list. Values higher or equal to the specified by "
                  "this list will be ignored");
  // TODO:    "If a maximal target workspace range is lower, then one of
  // specified here, the target workspace range will be used instead" );

  // Box controller properties. These are the defaults
  this->initBoxControllerProps("5" /*SplitInto*/, 1000 /*SplitThreshold*/,
                               20 /*MaxRecursionDepth*/);
  // additional box controller settings property.
  auto mustBeMoreThan1 = boost::make_shared<BoundedValidator<int>>();
  mustBeMoreThan1->setLower(1);

  declareProperty(
      make_unique<PropertyWithValue<int>>("MinRecursionDepth", 1,
                                          mustBeMoreThan1),
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
      make_unique<PropertyWithValue<bool>>("TopLevelSplitting", false,
                                           Direction::Input),
      "This option causes a split of the top level, i.e. level0, of 50 for the "
      "first four dimensions.");

  declareProperty(
      make_unique<FileProperty>("Filename", "", FileProperty::OptionalSave,
                                ".nxs"),
      "The name of the Nexus file to write, as a full or relative path.\n"
      "Only used if FileBackEnd is true.");
  setPropertySettings("Filename", make_unique<EnabledWhenProperty>(
                                      "FileBackEnd", IS_EQUAL_TO, "1"));

  declareProperty("FileBackEnd", false,
                  "If true, Filename must also be specified. The algorithm "
                  "will create the specified file in addition to an output "
                  "workspace. The workspace will load data from the file on "
                  "demand in order to reduce memory use.");

  std::vector<std::string> converterType{"Default", "Indexed"};

  auto loadTypeValidator =
      boost::make_shared<StringListValidator>(converterType);
  declareProperty("ConverterType", "Default", loadTypeValidator,
                  "[Default, Indexed], indexed is the experimental type that "
                  "can speedup the conversion process"
                  "for the big files using the indexing.");
}
//----------------------------------------------------------------------------------------------

const std::string ConvertToMD::name() const { return "ConvertToMD"; }

int ConvertToMD::version() const { return 1; }

std::map<std::string, std::string> ConvertToMD::validateInputs() {
  std::map<std::string, std::string> result;

  const std::string treeBuilderType = this->getProperty("ConverterType");
  const bool topLevelSplittingChecked = this->getProperty("TopLevelSplitting");
  std::vector<int> split_into = this->getProperty("SplitInto");
  const std::string filename = this->getProperty("Filename");
  const bool fileBackEnd = this->getProperty("FileBackEnd");

  if (fileBackEnd && filename.empty()) {
    result["Filename"] = "Filename must be given if FileBackEnd is required.";
  }

  if (treeBuilderType.find("Indexed") != std::string::npos) {
    if (fileBackEnd)
      result["ConverterType"] += "No file back end implemented "
                                 "for indexed version of algorithm. ";

    if (topLevelSplittingChecked)
      result["ConverterType"] +=
          "The usage of top level splitting is "
          "not possible for indexed version of algorithm. ";

    bool validSplitInfo = ConvToMDEventsWSIndexing::isSplitValid(split_into);
    if (!validSplitInfo)
      result["ConverterType"] +=
          "The split parameter should be the same for"
          " all dimensions and be equal the power of 2"
          " (2 ,4, 8, 16,..) for indexed version of algorithm. ";
  }

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
    m_OutWSWrapper = boost::make_shared<MDEventWSWrapper>();

  // -------- get Input workspace
  m_InWS2D = getProperty("InputWorkspace");

  const std::string out_filename = this->getProperty("Filename");
  const bool fileBackEnd = this->getProperty("FileBackEnd");

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

  // Sanity check some options
  if (QModReq != MDTransfQ3D().transfID()) {
    MDWSTransform transform;
    const std::string autoSelect =
        transform.getTargetFrames()[CnvrtToMD::AutoSelect];
    if (QFrame != autoSelect) {
      g_log.warning("Q3DFrames value ignored with QDimensions != " +
                    MDTransfQ3D().transfID());
      QFrame = autoSelect;
    }
    const std::string noScaling =
        transform.getQScalings()[CnvrtToMD::NoScaling];
    if (convertTo_ != noScaling) {
      g_log.warning("QConversionScales value ignored with QDimensions != " +
                    MDTransfQ3D().transfID());
      convertTo_ = noScaling;
    }
  }

  // Build the target ws description as function of the input & output ws and
  // the parameters, supplied to the algorithm
  MDWSDescription targWSDescr;
  // get workspace parameters and build target workspace description, report if
  // there is need to build new target MD workspace
  bool createNewTargetWs =
      buildTargetWSDescription(spws, QModReq, dEModReq, otherDimNames, dimMin,
                               dimMax, QFrame, convertTo_, targWSDescr);

  // create and initiate new workspace or set up existing workspace as a target.
  if (createNewTargetWs) // create new
    spws = this->createNewMDWorkspace(targWSDescr, fileBackEnd, out_filename);
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
  ConvToMDSelector::ConverterType convType =
      getPropertyValue("ConverterType") == "Indexed"
          ? ConvToMDSelector::INDEXED
          : ConvToMDSelector::DEFAULT;
  ConvToMDSelector AlgoSelector(convType);
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

  // Set the normalization of the event workspace
  m_Convertor->setDisplayNormalization(spws, m_InWS2D);

  if (fileBackEnd) {
    auto savemd = this->createChildAlgorithm("SaveMD");
    savemd->setProperty("InputWorkspace", spws);
    savemd->setPropertyValue("Filename", out_filename);
    savemd->setProperty("UpdateFileBackEnd", true);
    savemd->setProperty("MakeFileBacked", false);
    savemd->executeAsChildAlg();
  }

  // JOB COMPLETED:
  setProperty("OutputWorkspace",
              boost::dynamic_pointer_cast<IMDEventWorkspace>(spws));
  // free the algorithm from the responsibility for the target workspace to
  // allow it to be deleted if necessary
  m_OutWSWrapper->releaseWorkspace();
  // free up the sp to the input workspace, which would be deleted if nobody
  // needs it any more;
  m_InWS2D.reset();
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
void ConvertToMD::addExperimentInfo(API::IMDEventWorkspace_sptr &mdEventWS,
                                    MDWSDescription &targWSDescr) const {
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
  bool detector_found(false);
  const auto &spectrumInfo = m_InWS2D->spectrumInfo();
  for (size_t i = 0; i < m_InWS2D->getNumberHistograms(); ++i) {
    if (spectrumInfo.hasDetectors(i) && !spectrumInfo.isMonitor(i)) {
      spectra_index = i;
      detector_found = true;
      g_log.debug() << "Using spectra N " << i
                    << " as the source of the bin "
                       "boundaries for the resolution corrections \n";
      break;
    }
  }
  if (!detector_found) {
    g_log.information()
        << "No spectra in the workspace have detectors associated "
           "with them. Storing bin boundaries from first spectrum for"
           "resolution calculation\n";
  }

  // retrieve representative bin boundaries
  auto binBoundaries = m_InWS2D->x(spectra_index);

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
      for (auto &binBoundary : binBoundaries) {
        binBoundary = unitConv.convertUnits(binBoundary);
      }
    }
    // sort bin boundaries in case if unit transformation have swapped them.
    if (binBoundaries[0] > binBoundaries.back()) {
      g_log.information() << "Bin boundaries are not arranged monotonously. "
                             "Sorting performed\n";
      std::sort(binBoundaries.begin(), binBoundaries.end());
    }
  }

  // Replacement for SpectraDetectorMap::createIDGroupsMap using the ISpectrum
  // objects instead
  auto mapping = boost::make_shared<det2group_map>();
  for (size_t i = 0; i < m_InWS2D->getNumberHistograms(); ++i) {
    const auto &dets = m_InWS2D->getSpectrum(i).getDetectorIDs();
    if (!dets.empty())
      mapping->emplace(*dets.begin(), dets);
  }

  // The last experiment info should always be the one that refers
  // to latest converting workspace. All others should have had this
  // information set already
  uint16_t nexpts = mdEventWS->getNumExperimentInfo();
  if (nexpts > 0) {
    ExperimentInfo_sptr expt =
        mdEventWS->getExperimentInfo(static_cast<uint16_t>(nexpts - 1));
    expt->mutableRun().storeHistogramBinBoundaries(binBoundaries.rawData());
    expt->cacheDetectorGroupings(*mapping);
  }
}

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
is ignored in any other case
* @param convertTo_  -- The parameter describing Q-scaling transformations
* @param targWSDescr -- the resulting class used to interpret all parameters
together and used to describe selected transformation.
*/
bool ConvertToMD::buildTargetWSDescription(
    API::IMDEventWorkspace_sptr spws, const std::string &QModReq,
    const std::string &dEModReq, const std::vector<std::string> &otherDimNames,
    std::vector<double> &dimMin, std::vector<double> &dimMax,
    const std::string &QFrame, const std::string &convertTo_,
    MDAlgorithms::MDWSDescription &targWSDescr) {
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

  double m_AbsMin = getProperty("AbsMinQ");
  targWSDescr.setAbsMin(m_AbsMin);

  // Set optional projections for Q3D mode
  MDAlgorithms::MDWSTransform MsliceProj;
  if (QModReq == MDTransfQ3D().transfID()) {
    try {
      // otherwise input uv are ignored -> later it can be modified to set ub
      // matrix if no given, but this may over-complicate things.
      MsliceProj.setUVvectors(getProperty("UProj"), getProperty("VProj"),
                              getProperty("WProj"));
    } catch (std::invalid_argument &) {
      g_log.warning() << "The projections are coplanar. Will use defaults "
                         "[1,0,0],[0,1,0] and [0,0,1]\n";
    }
  } else {
    auto warnIfSet = [this](const std::string &propName) {
      Property *prop = this->getProperty(propName);
      if (!prop->isDefault()) {
        g_log.warning(propName + " value ignored with QDimensions != " +
                      MDTransfQ3D().transfID());
      }
    };
    for (const auto &name : {"UProj", "VProj", "WProj"}) {
      warnIfSet(name);
    }
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
    MDAlgorithms::MDWSDescription oldWSDescr;
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
 * @param targWSDescr :: Description of workspace to create
 * @param filebackend :: true if the workspace will have a file back end
 * @param filename :: file to use for file back end of workspace
 * @return :: Shared pointer for the created workspace
 */
API::IMDEventWorkspace_sptr
ConvertToMD::createNewMDWorkspace(const MDWSDescription &targWSDescr,
                                  const bool filebackend,
                                  const std::string &filename) {
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
  if (filebackend) {
    setupFileBackend(filename, m_OutWSWrapper->pWorkspace());
  }

  // Check if the user want sto force a top level split or not
  bool topLevelSplittingChecked = this->getProperty("TopLevelSplitting");

  if (topLevelSplittingChecked) {
    // Perform initial split with the forced settings
    setupTopLevelSplitting(bc);
  }

  // split boxes;
  spws->splitBox();

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
 * Splits the top level box at level 0 into a defined number of subboxes for the
 * the first level.
 * @param bc A pointer to the box controller.
 */
void ConvertToMD::setupTopLevelSplitting(Mantid::API::BoxController_sptr bc) {
  const size_t topLevelSplitSetting = 50;
  const size_t dimCutoff = 4;

  // Set the Top level splitting
  for (size_t dim = 0; dim < bc->getNDims(); dim++) {
    if (dim < dimCutoff) {
      bc->setSplitTopInto(dim, topLevelSplitSetting);
    } else {
      bc->setSplitTopInto(dim, bc->getSplitInto(dim));
    }
  }
}

/**Check if the target workspace new or exists and we need to create new
 *workspace
 *@param spws -- shared pointer to target MD workspace, which can be undefined
 *if the workspace does not exist
 *
 *@returns true if one needs to create new workspace and false otherwise
 */
bool ConvertToMD::doWeNeedNewTargetWorkspace(API::IMDEventWorkspace_sptr spws) {

  bool createNewWs(false);
  if (!spws) {
    createNewWs = true;
  } else {
    bool shouldOverwrite = getProperty("OverwriteExisting");
    createNewWs = shouldOverwrite;
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
  auto iEmode = Kernel::DeltaEMode::fromString(dEMode);
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
            << " equal or exceeds max value:" << maxVal[i] << '\n';
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

  childAlg->setProperty("InputWorkspace", inWS);
  childAlg->setProperty("QDimensions", QMode);
  childAlg->setProperty("dEAnalysisMode", dEMode);
  childAlg->setProperty("Q3DFrames", QFrame);
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
    } else {
      MDHistoDimensionBuilder::resizeToFitMDBox(minVal[i], maxVal[i]);
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

/**
 * Setup the filebackend for the output workspace. It assumes that the
 * box controller has already been initialized
 * @param filebackPath :: Path to the file used for backend storage
 * @param outputWS :: Workspace on which to set the file back end
 */
void ConvertToMD::setupFileBackend(
    std::string filebackPath, Mantid::API::IMDEventWorkspace_sptr outputWS) {
  using DataObjects::BoxControllerNeXusIO;
  auto savemd = this->createChildAlgorithm("SaveMD", 0.01, 0.05, true);
  savemd->setProperty("InputWorkspace", outputWS);
  savemd->setPropertyValue("Filename", filebackPath);
  savemd->setProperty("UpdateFileBackEnd", false);
  savemd->setProperty("MakeFileBacked", false);
  savemd->executeAsChildAlg();

  // create file-backed box controller
  auto boxControllerMem = outputWS->getBoxController();
  auto boxControllerIO =
      boost::make_shared<BoxControllerNeXusIO>(boxControllerMem.get());
  boxControllerMem->setFileBacked(boxControllerIO, filebackPath);
  outputWS->setFileBacked();
  boxControllerMem->getFileIO()->setWriteBufferSize(1000000);
}

} // namespace MDAlgorithms
} // namespace Mantid
