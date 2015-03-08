#include "MantidMDAlgorithms/ConvertToReflectometryQ.h"

#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/MDEventFactory.h"

#include "MantidMDAlgorithms/ReflectometryTransformKiKf.h"
#include "MantidMDAlgorithms/ReflectometryTransformQxQz.h"
#include "MantidMDAlgorithms/ReflectometryTransformP.h"

#include <boost/shared_ptr.hpp>
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
}

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertToReflectometryQ)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ConvertToReflectometryQ::ConvertToReflectometryQ() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ConvertToReflectometryQ::~ConvertToReflectometryQ() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string ConvertToReflectometryQ::name() const {
  return "ConvertToReflectometryQ";
};

/// Algorithm's version for identification. @see Algorithm::version
int ConvertToReflectometryQ::version() const { return 1; };

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

  declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "",
                                                         Direction::Input,
                                                         compositeValidator),
                  "An input workspace in wavelength");

  std::vector<std::string> propOptions;
  propOptions.push_back(qSpaceTransform());
  propOptions.push_back(pSpaceTransform());
  propOptions.push_back(kSpaceTransform());

  declareProperty(
      "OutputDimensions", "Q (lab frame)",
      boost::make_shared<StringListValidator>(propOptions),
      "What will be the dimensions of the output workspace?\n"
      "  Q (lab frame): Wave-vector change of the lattice in the lab frame.\n"
      "  P (lab frame): Momentum in the sample frame.\n"
      "  K initial and final vectors in the z plane.");

  declareProperty(
      new Kernel::PropertyWithValue<bool>("OverrideIncidentTheta", false),
      "Use the provided incident theta value.");

  declareProperty(
      new PropertyWithValue<double>("IncidentTheta", -1),
      "An input incident theta value specified in degrees."
      "Optional input value for the incident theta specified in degrees.");

  std::vector<double> extents(4, 0);
  extents[0] = -50;
  extents[1] = +50;
  extents[2] = -50;
  extents[3] = +50;
  declareProperty(new ArrayProperty<double>("Extents", extents),
                  "A comma separated list of min, max for each dimension. "
                  "Takes four values in the form dim_0_min, dim_0_max, "
                  "dim_1_min, dim_1_max,\n"
                  "specifying the extents of each dimension. Optional, default "
                  "+-50 in each dimension.");

  setPropertySettings("IncidentTheta",
                      new Kernel::EnabledWhenProperty("OverrideIncidentTheta",
                                                      IS_EQUAL_TO, "1"));

  declareProperty(
      new Kernel::PropertyWithValue<bool>("OutputAsMDWorkspace", true),
      "Generate the output as a MDWorkspace, otherwise a Workspace2D is "
      "returned.");

  declareProperty(new WorkspaceProperty<IMDWorkspace>("OutputWorkspace", "",
                                                      Direction::Output),
                  "Output 2D Workspace.");

  declareProperty(new Kernel::PropertyWithValue<int>("NumberBinsQx", 100),
                  "The number of bins along the qx axis. Optional and only "
                  "applies to 2D workspaces. Defaults to 100.");
  declareProperty(new Kernel::PropertyWithValue<int>("NumberBinsQz", 100),
                  "The number of bins along the qx axis. Optional and only "
                  "applies to 2D workspaces. Defaults to 100.");
  setPropertySettings(
      "NumberBinsQx",
      new EnabledWhenProperty("OutputAsMDWorkspace", IS_NOT_DEFAULT));
  setPropertySettings(
      "NumberBinsQz",
      new EnabledWhenProperty("OutputAsMDWorkspace", IS_NOT_DEFAULT));

  // Create box controller properties.
  this->initBoxControllerProps("2,2", 50, 10);

  // Only show box controller properties when a md workspace is returned.
  setPropertySettings(
      "SplitInto", new EnabledWhenProperty("OutputAsMDWorkspace", IS_DEFAULT));
  setPropertySettings("SplitThreshold", new EnabledWhenProperty(
                                            "OutputAsMDWorkspace", IS_DEFAULT));
  setPropertySettings(
      "MaxRecursionDepth",
      new EnabledWhenProperty("OutputAsMDWorkspace", IS_DEFAULT));
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
                                                     // implemented.

  // Extract the incient theta angle from the logs if a user provided one is not
  // given.
  if (!bUseOwnIncidentTheta) {
    const Mantid::API::Run &run = inputWs->run();
    try {
      Property *p = run.getLogData("stheta");
      auto incidentThetas =
          dynamic_cast<Mantid::Kernel::TimeSeriesProperty<double> *>(p);
      incidentTheta =
          incidentThetas->valuesAsVector().back(); // Not quite sure what to do
                                                   // with the time series for
                                                   // stheta
      checkIncidentTheta(incidentTheta);
      std::stringstream stream;
      stream << "Extracted initial theta value of: " << incidentTheta;
      g_log.information(stream.str());
    } catch (Exception::NotFoundError &) {
      throw std::runtime_error(
          "The input workspace does not have a stheta log value.");
    }
  }

  // Min max extent values.
  const double dim0min = extents[0];
  const double dim0max = extents[1];
  const double dim1min = extents[2];
  const double dim1max = extents[3];

  BoxController_sptr bc = boost::make_shared<BoxController>(2);
  this->setBoxController(bc);

  // Select the transform strategy.
  ReflectometryTransform_sptr transform;

  if (outputDimensions == qSpaceTransform()) {
    transform = boost::make_shared<ReflectometryTransformQxQz>(
        dim0min, dim0max, dim1min, dim1max, incidentTheta, numberOfBinsQx,
        numberOfBinsQz);
  } else if (outputDimensions == pSpaceTransform()) {
    transform = boost::make_shared<ReflectometryTransformP>(
        dim0min, dim0max, dim1min, dim1max, incidentTheta, numberOfBinsQx,
        numberOfBinsQz);
  } else {
    transform = boost::make_shared<ReflectometryTransformKiKf>(
        dim0min, dim0max, dim1min, dim1max, incidentTheta, numberOfBinsQx,
        numberOfBinsQz);
  }

  IMDWorkspace_sptr outputWS;

  if (outputAsMDWorkspace) {
    auto outputMDWS = transform->executeMD(inputWs, bc);

    // Copy ExperimentInfo (instrument, run, sample) to the output WS
    ExperimentInfo_sptr ei(inputWs->cloneExperimentInfo());
    outputMDWS->addExperimentInfo(ei);
    outputWS = outputMDWS;
  } else {
    auto outputWS2D = transform->execute(inputWs);
    outputWS2D->copyExperimentInfoFrom(inputWs.get());
    outputWS = outputWS2D;
  }

  // Execute the transform and bind to the output.
  setProperty("OutputWorkspace", outputWS);
}

} // namespace Mantid
} // namespace MDAlgorithms
