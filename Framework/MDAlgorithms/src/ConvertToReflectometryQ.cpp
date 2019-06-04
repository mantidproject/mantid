// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/ConvertToReflectometryQ.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceUnitValidator.h"

#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

#include "MantidGeometry/MDGeometry/GeneralFrame.h"
#include "MantidGeometry/MDGeometry/QLab.h"

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/Unit.h"

#include "MantidMDAlgorithms/ReflectometryTransformKiKf.h"
#include "MantidMDAlgorithms/ReflectometryTransformP.h"
#include "MantidMDAlgorithms/ReflectometryTransformQxQz.h"

#include <boost/make_shared.hpp>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

/*Non member helpers*/
namespace {
/*
Transform to q-space label:
@return: associated id/label
*/
std::string qSpaceTransform() { return "Q (lab frame)"; }

/*
Transform to p-space label:
@return: associated id/label
*/
std::string pSpaceTransform() { return "P (lab frame)"; }

/*
Transform to k-space label:
@return: associated id/label
*/
std::string kSpaceTransform() { return "K (incident, final)"; }

/*
Center point rebinning
@return: associated id/label
*/
std::string centerTransform() { return "Centre"; }

/*
Normalised polygon rebinning
@return: associated id/label
*/
std::string normPolyTransform() { return "NormalisedPolygon"; }

/*
Check that the input workspace is of the correct type.
@param: inputWS: The input workspace.
@throw: runtime_error if the units do not appear to be correct/compatible with
the algorithm.
*/
void checkInputWorkspace(Mantid::API::MatrixWorkspace_const_sptr inputWs) {
  auto spectraAxis = inputWs->getAxis(1);
  const std::string label = spectraAxis->unit()->label();
  const std::string expectedLabel = "degrees";
  if (expectedLabel != label) {
    std::string message =
        "Spectra axis should have units of degrees. Instead found: " + label;
    throw std::runtime_error(message);
  }
}

/*
Check the extents.
@param extents: A vector containing all the extents.
@throw: runtime_error if the extents appear to be incorrect.
*/
void checkExtents(const std::vector<double> &extents) {
  if (extents.size() != 4) {
    throw std::runtime_error(
        "Four comma separated extents inputs should be provided");
  }
  if ((extents[0] >= extents[1]) || (extents[2] >= extents[3])) {
    throw std::runtime_error(
        "Extents must be provided min, max with min less than max!");
  }
}

/*
Check the incident theta inputs.
@param bUseOwnIncidentTheta: True if the user has requested to provide their own
incident theta value.
@param theta: The proposed incident theta value provided by the user.
@throw: logic_error if the theta value is out of range.
*/
void checkCustomThetaInputs(const bool bUseOwnIncidentTheta,
                            const double &theta) {
  if (bUseOwnIncidentTheta) {
    if (theta < 0 || theta > 90) {
      throw std::logic_error("Overriding incident theta is out of range");
    }
  }
}

/*
General check for the indient theta.
@param theta: The proposed incident theta value.
@throw: logic_error if the theta value is out of range.
*/
void checkIncidentTheta(const double &theta) {
  if (theta < 0 || theta > 90) {
    throw std::logic_error("Incident theta is out of range");
  }
}

/*
Check for the output dimensionality.
@param outputDimensions : requested output dimensionality
@throw: runtime_errror if the dimensionality is not supported.
*/
void checkOutputDimensionalityChoice(const std::string &outputDimensions) {
  if (outputDimensions != qSpaceTransform() &&
      outputDimensions != kSpaceTransform() &&
      outputDimensions != pSpaceTransform()) {
    throw std::runtime_error("Unknown transformation");
  }
}

/*
Get the value of theta from the logs
@param inputWs : the input workspace
@return : theta found in the logs
@throw: runtime_errror if 'stheta' was not found.
*/
double getThetaFromLogs(MatrixWorkspace_sptr inputWs) {

  double theta = -1.;
  const Mantid::API::Run &run = inputWs->run();
  try {
    Property *p = run.getLogData("stheta");
    auto incidentThetas =
        dynamic_cast<Mantid::Kernel::TimeSeriesProperty<double> *>(p);
    if (!incidentThetas) {
      throw std::runtime_error("stheta log not found");
    }
    theta =
        incidentThetas->valuesAsVector().back(); // Not quite sure what to do
                                                 // with the time series for
                                                 // stheta
  } catch (Exception::NotFoundError &) {
    return theta;
  }
  return theta;
}
} // namespace

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertToReflectometryQ)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string ConvertToReflectometryQ::name() const {
  return "ConvertToReflectometryQ";
}

/// Algorithm's version for identification. @see Algorithm::version
int ConvertToReflectometryQ::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ConvertToReflectometryQ::category() const {
  return "Reflectometry";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ConvertToReflectometryQ::init() {
  auto compositeValidator = boost::make_shared<CompositeValidator>();
  compositeValidator->add(
      boost::make_shared<API::WorkspaceUnitValidator>("Wavelength"));
  compositeValidator->add(boost::make_shared<API::HistogramValidator>());

  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "InputWorkspace", "", Direction::Input, compositeValidator),
      "An input workspace in wavelength");

  std::vector<std::string> propOptions{qSpaceTransform(), pSpaceTransform(),
                                       kSpaceTransform()};

  declareProperty(
      "OutputDimensions", "Q (lab frame)",
      boost::make_shared<StringListValidator>(propOptions),
      "What will be the dimensions of the output workspace?\n"
      "  Q (lab frame): Wave-vector change of the lattice in the lab frame.\n"
      "  P (lab frame): Momentum in the sample frame.\n"
      "  K initial and final vectors in the z plane.");

  std::vector<std::string> transOptions{centerTransform(), normPolyTransform()};

  declareProperty("Method", centerTransform(),
                  boost::make_shared<StringListValidator>(transOptions),
                  "What method should be used for the axis transformation?\n"
                  "  Centre: Use center point rebinning.\n"
                  "  NormalisedPolygon: Use normalised polygon rebinning.");

  declareProperty(std::make_unique<Kernel::PropertyWithValue<bool>>(
                      "OverrideIncidentTheta", false),
                  "Use the provided incident theta value.");

  declareProperty(
      std::make_unique<PropertyWithValue<double>>("IncidentTheta", -1),
      "An input incident theta value specified in degrees."
      "Optional input value for the incident theta specified in degrees.");

  std::vector<double> extents{-50, +50, -50, +50};
  declareProperty(
      std::make_unique<ArrayProperty<double>>("Extents", std::move(extents)),
      "A comma separated list of min, max for each dimension. "
      "Takes four values in the form dim_0_min, dim_0_max, "
      "dim_1_min, dim_1_max,\n"
      "specifying the extents of each dimension. Optional, default "
      "+-50 in each dimension.");

  setPropertySettings("IncidentTheta",
                      std::make_unique<Kernel::EnabledWhenProperty>(
                          "OverrideIncidentTheta", IS_EQUAL_TO, "1"));

  declareProperty(
      std::make_unique<Kernel::PropertyWithValue<bool>>("OutputAsMDWorkspace", true),
      "Generate the output as a MDWorkspace, otherwise a Workspace2D is "
      "returned.");

  declareProperty(std::make_unique<WorkspaceProperty<IMDWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Output 2D Workspace.");

  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>(
                      "OutputVertexes", "", Direction::Output),
                  "Output TableWorkspace with vertex information. See "
                  "DumpVertexes property.");

  declareProperty(
      std::make_unique<Kernel::PropertyWithValue<int>>("NumberBinsQx", 100),
      "The number of bins along the qx axis. Optional and only "
      "applies to 2D workspaces. Defaults to 100.");
  declareProperty(
      std::make_unique<Kernel::PropertyWithValue<int>>("NumberBinsQz", 100),
      "The number of bins along the qx axis. Optional and only "
      "applies to 2D workspaces. Defaults to 100.");

  declareProperty(
      std::make_unique<Kernel::PropertyWithValue<bool>>("DumpVertexes", false),
      "If set, with 2D rebinning, the intermediate vertexes for "
      "each polygon will be written out for debugging purposes. "
      "Creates a second output table workspace.");

  setPropertySettings(
      "NumberBinsQx",
      std::make_unique<EnabledWhenProperty>("OutputAsMDWorkspace", IS_NOT_DEFAULT));
  setPropertySettings(
      "NumberBinsQz",
      std::make_unique<EnabledWhenProperty>("OutputAsMDWorkspace", IS_NOT_DEFAULT));

  // Create box controller properties.
  this->initBoxControllerProps("2,2", 50, 10);

  // Only show box controller properties when a md workspace is returned.
  setPropertySettings("SplitInto", std::make_unique<EnabledWhenProperty>(
                                       "OutputAsMDWorkspace", IS_DEFAULT));
  setPropertySettings("SplitThreshold", std::make_unique<EnabledWhenProperty>(
                                            "OutputAsMDWorkspace", IS_DEFAULT));
  setPropertySettings(
      "MaxRecursionDepth",
      std::make_unique<EnabledWhenProperty>("OutputAsMDWorkspace", IS_DEFAULT));
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ConvertToReflectometryQ::exec() {
  Mantid::API::MatrixWorkspace_sptr inputWs = getProperty("InputWorkspace");
  const bool bUseOwnIncidentTheta = getProperty("OverrideIncidentTheta");
  const std::vector<double> extents = getProperty("Extents");
  double incidentTheta = getProperty("IncidentTheta");
  const std::string outputDimensions = getPropertyValue("OutputDimensions");
  const std::string transMethod = getPropertyValue("Method");
  const bool outputAsMDWorkspace = getProperty("OutputAsMDWorkspace");
  const int numberOfBinsQx = getProperty("NumberBinsQx");
  const int numberOfBinsQz = getProperty("NumberBinsQz");

  // Validation of input parameters
  checkInputWorkspace(inputWs);
  checkExtents(extents);
  checkCustomThetaInputs(bUseOwnIncidentTheta, incidentTheta);
  checkOutputDimensionalityChoice(outputDimensions); // TODO: This check can be
                                                     // retired as soon as all
                                                     // transforms have been

  // Extract the incident theta angle from the logs
  double thetaFromLogs = getThetaFromLogs(inputWs);
  if (!bUseOwnIncidentTheta) {
    // Use the incident theta angle from the logs if a user provided one is not
    // given.
    checkIncidentTheta(thetaFromLogs);
    incidentTheta = thetaFromLogs;
  }

  // Correct the detectors according to theta read from logs
  inputWs = correctDetectors(inputWs, thetaFromLogs);

  // Min max extent values.
  const double dim0min = extents[0];
  const double dim0max = extents[1];
  const double dim1min = extents[2];
  const double dim1max = extents[3];

  BoxController_sptr bc = boost::make_shared<BoxController>(2);
  this->setBoxController(bc);

  // Select the transform strategy and an appropriate MDFrame
  ReflectometryTransform_sptr transform;
  Mantid::Geometry::MDFrame_uptr frame;
  if (outputDimensions == qSpaceTransform()) {
    transform = boost::make_shared<ReflectometryTransformQxQz>(
        dim0min, dim0max, dim1min, dim1max, incidentTheta, numberOfBinsQx,
        numberOfBinsQz);
    frame.reset(new Mantid::Geometry::QLab);
  } else if (outputDimensions == pSpaceTransform()) {
    transform = boost::make_shared<ReflectometryTransformP>(
        dim0min, dim0max, dim1min, dim1max, incidentTheta, numberOfBinsQx,
        numberOfBinsQz);
    frame.reset(new Mantid::Geometry::GeneralFrame(
        "P", Mantid::Kernel::InverseAngstromsUnit().getUnitLabel()));
  } else {
    transform = boost::make_shared<ReflectometryTransformKiKf>(
        dim0min, dim0max, dim1min, dim1max, incidentTheta, numberOfBinsQx,
        numberOfBinsQz);
    frame.reset(new Mantid::Geometry::GeneralFrame(
        "KiKf", Mantid::Kernel::InverseAngstromsUnit().getUnitLabel()));
  }

  IMDWorkspace_sptr outputWS;

  TableWorkspace_sptr vertexes =
      boost::make_shared<Mantid::DataObjects::TableWorkspace>();
  Progress transSelectionProg(this, 0.0, 0.1, 2);
  if (outputAsMDWorkspace) {
    transSelectionProg.report("Choosing Transformation");
    if (transMethod == centerTransform()) {
      auto outputMDWS = transform->executeMD(inputWs, bc, std::move(frame));
      Progress transPerformProg(this, 0.1, 0.7, 5);
      transPerformProg.report("Performed transformation");
      // Copy ExperimentInfo (instrument, run, sample) to the output WS
      ExperimentInfo_sptr ei(inputWs->cloneExperimentInfo());
      outputMDWS->addExperimentInfo(ei);
      outputWS = outputMDWS;
    } else if (transMethod == normPolyTransform()) {
      Progress transPerformProg(this, 0.1, 0.7, 5);
      const bool dumpVertexes = this->getProperty("DumpVertexes");
      auto vertexesTable = vertexes;
      // perform the normalised polygon transformation
      transPerformProg.report("Performing Transformation");
      auto normPolyTrans = transform->executeNormPoly(
          inputWs, vertexesTable, dumpVertexes, outputDimensions);
      // copy any experiment info from input workspace
      normPolyTrans->copyExperimentInfoFrom(inputWs.get());
      // produce MDHistoWorkspace from normPolyTrans workspace.
      Progress outputToMDProg(this, 0.7, 0.75, 10);
      auto outputMDWS = transform->executeMDNormPoly(normPolyTrans);
      ExperimentInfo_sptr ei(normPolyTrans->cloneExperimentInfo());
      outputMDWS->addExperimentInfo(ei);
      outputWS = outputMDWS;
      outputToMDProg.report("Successfully output to MD");
    } else {
      throw std::runtime_error("Unknown rebinning method: " + transMethod);
    }
  } else if (transMethod == normPolyTransform()) {
    transSelectionProg.report("Choosing Transformation");
    Progress transPerformProg(this, 0.1, 0.7, 5);
    const bool dumpVertexes = this->getProperty("DumpVertexes");
    auto vertexesTable = vertexes;
    // perform the normalised polygon transformation
    transPerformProg.report("Performing Transformation");
    auto output2DWS = transform->executeNormPoly(
        inputWs, vertexesTable, dumpVertexes, outputDimensions);
    // copy any experiment info from input workspace
    output2DWS->copyExperimentInfoFrom(inputWs.get());
    outputWS = output2DWS;
    transPerformProg.report("Transformation Complete");
  } else if (transMethod == centerTransform()) {
    transSelectionProg.report("Choosing Transformation");
    Progress transPerformProg(this, 0.1, 0.7, 5);
    transPerformProg.report("Performing Transformation");
    auto output2DWS = transform->execute(inputWs);
    output2DWS->copyExperimentInfoFrom(inputWs.get());
    outputWS = output2DWS;
  } else {
    throw std::runtime_error("Unknown rebinning method: " + transMethod);
  }

  // Execute the transform and bind to the output.
  setProperty("OutputWorkspace", outputWS);
  setProperty("OutputVertexes", vertexes);
  Progress setPropertyProg(this, 0.8, 1.0, 2);
  setPropertyProg.report("Success");
}

MatrixWorkspace_sptr
ConvertToReflectometryQ::correctDetectors(MatrixWorkspace_sptr inputWs,
                                          const double theta) {

  if (theta < 0)
    return inputWs;

  // Obtain the detector IDs that correspond to spectra in the input workspace
  std::set<Mantid::detid_t> detectorIDs;
  for (size_t sp = 0; sp < inputWs->getNumberHistograms(); sp++) {
    auto ids = inputWs->getSpectrum(sp).getDetectorIDs();
    detectorIDs.insert(ids.begin(), ids.end());
  }

  // Move the parent component of the selected detectors
  std::set<std::string> componentsToMove;
  const auto &instrument = inputWs->getInstrument();
  for (const auto &id : detectorIDs) {
    auto detector = instrument->getDetector(id);
    auto parent = detector->getParent();
    if (parent) {
      auto parentType = parent->type();
      auto detectorName = (parentType == "Instrument") ? detector->getName()
                                                       : parent->getName();
      componentsToMove.insert(detectorName);
    }
  }

  MatrixWorkspace_sptr outWS = inputWs;
  for (const auto &component : componentsToMove) {
    IAlgorithm_sptr alg =
        createChildAlgorithm("SpecularReflectionPositionCorrect");
    alg->setProperty("InputWorkspace", outWS);
    alg->setProperty("TwoTheta", theta);
    alg->setProperty("DetectorComponentName", component);
    alg->execute();
    outWS = alg->getProperty("OutputWorkspace");
  }

  return outWS;
}

} // namespace MDAlgorithms
} // namespace Mantid
