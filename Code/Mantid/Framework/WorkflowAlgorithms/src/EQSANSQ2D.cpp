//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidWorkflowAlgorithms/EQSANSQ2D.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "Poco/NumberFormatter.h"
#include "MantidWorkflowAlgorithms/EQSANSInstrument.h"

namespace Mantid {
namespace WorkflowAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(EQSANSQ2D)

using namespace Kernel;
using namespace API;
using namespace Geometry;

void EQSANSQ2D::init() {
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  declareProperty(new WorkspaceProperty<>("InputWorkspace", "",
                                          Direction::Input, wsValidator),
                  "Workspace to calculate I(qx,qy) from");
  declareProperty("OutputWorkspace", "", Direction::Input);
  declareProperty("NumberOfBins", 100,
                  "Number of bins in each dimension of the 2D output",
                  Kernel::Direction::Input);
  declareProperty("OutputMessage", "", Direction::Output);
}

/// Returns the value of a run property from a given workspace
/// @param inputWS :: input workspace
/// @param pname :: name of the property to retrieve
double getRunProperty(MatrixWorkspace_sptr inputWS, const std::string &pname) {
  Mantid::Kernel::Property *prop = inputWS->run().getProperty(pname);
  Mantid::Kernel::PropertyWithValue<double> *dp =
      dynamic_cast<Mantid::Kernel::PropertyWithValue<double> *>(prop);
  return *dp;
}

/// Execute algorithm
void EQSANSQ2D::exec() {
  Progress progress(this, 0.0, 1.0, 3);
  progress.report("Setting up I(qx,Qy) calculation");

  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  const int nbins = getProperty("NumberOfBins");

  // If the OutputWorkspace property was not given, use the
  // name of the input workspace as the base name for the output
  std::string outputWSName = getPropertyValue("OutputWorkspace");
  if (outputWSName.size() == 0) {
    outputWSName = inputWS->getName();
  }

  // Determine whether we need frame skipping or not by checking the chopper
  // speed
  bool frame_skipping = false;
  const auto &run = inputWS->run();
  if (run.hasProperty("is_frame_skipping")) {
    auto prop = run.getProperty("is_frame_skipping");
    const auto &typeInfo = *(prop->type_info());
    if (typeInfo == typeid(long)) {
      frame_skipping =
          (run.getPropertyValueAsType<long>("is_frame_skipping") == 1);
    } else if (typeInfo == typeid(int)) {
      frame_skipping =
          (run.getPropertyValueAsType<int>("is_frame_skipping") == 1);
    } else
      g_log.warning() << "Unknown property type for is_frame_skipping\n";
  }

  // Get run properties necessary to calculate the input parameters to Qxy
  // Wavelength bandwidths
  double wavelength_min = 0.0;
  if (inputWS->run().hasProperty("wavelength_min"))
    wavelength_min = getRunProperty(inputWS, "wavelength_min");
  else if (inputWS->dataX(0).size() > 1)
    wavelength_min = (inputWS->dataX(1)[0] + inputWS->dataX(1)[1]) / 2.0;
  else if (inputWS->dataX(0).size() == 1)
    wavelength_min = inputWS->dataX(1)[0];
  else {
    g_log.error(
        "Can't determine the minimum wavelength for the input workspace.");
    throw std::invalid_argument(
        "Can't determine the minimum wavelength for the input workspace.");
  }

  double qmax = 0;
  if (inputWS->run().hasProperty("qmax")) {
    qmax = getRunProperty(inputWS, "qmax");
  } else {
    const double sample_detector_distance =
        getRunProperty(inputWS, "sample_detector_distance");

    const double nx_pixels =
        inputWS->getInstrument()->getNumberParameter("number-of-x-pixels")[0];
    const double ny_pixels =
        inputWS->getInstrument()->getNumberParameter("number-of-y-pixels")[0];
    const double pixel_size_x =
        inputWS->getInstrument()->getNumberParameter("x-pixel-size")[0];
    const double pixel_size_y =
        inputWS->getInstrument()->getNumberParameter("y-pixel-size")[0];

    const double beam_ctr_x = getRunProperty(inputWS, "beam_center_x");
    const double beam_ctr_y = getRunProperty(inputWS, "beam_center_y");

    double dxmax = pixel_size_x * std::max(beam_ctr_x, nx_pixels - beam_ctr_x);
    double dymax = pixel_size_y * std::max(beam_ctr_y, ny_pixels - beam_ctr_y);
    double maxdist = std::max(dxmax, dymax);

    qmax = 4 * M_PI / wavelength_min *
           std::sin(0.5 * std::atan(maxdist / sample_detector_distance));
  };

  if (frame_skipping) {
    // In frame skipping mode, treat each frame separately
    const double wavelength_max = getRunProperty(inputWS, "wavelength_max");
    const double wavelength_min_f2 =
        getRunProperty(inputWS, "wavelength_min_frame2");
    const double wavelength_max_f2 =
        getRunProperty(inputWS, "wavelength_max_frame2");

    // Frame 1
    std::string params = Poco::NumberFormatter::format(wavelength_min, 2) +
                         ",0.1," +
                         Poco::NumberFormatter::format(wavelength_max, 2);
    IAlgorithm_sptr rebinAlg = createChildAlgorithm("Rebin", 0.4, 0.5);
    rebinAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputWS);
    rebinAlg->setPropertyValue("Params", params);
    rebinAlg->setProperty("PreserveEvents", false);
    rebinAlg->executeAsChildAlg();

    IAlgorithm_sptr qxyAlg = createChildAlgorithm("Qxy", .5, .65);
    qxyAlg->setProperty<MatrixWorkspace_sptr>(
        "InputWorkspace", rebinAlg->getProperty("OutputWorkspace"));
    qxyAlg->setProperty<double>("MaxQxy", qmax);
    qxyAlg->setProperty<double>("DeltaQ", qmax / nbins);
    qxyAlg->setProperty<bool>("SolidAngleWeighting", false);
    qxyAlg->executeAsChildAlg();

    MatrixWorkspace_sptr qxy_output = qxyAlg->getProperty("OutputWorkspace");
    IAlgorithm_sptr replaceAlg =
        createChildAlgorithm("ReplaceSpecialValues", .65, 0.7);
    replaceAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", qxy_output);
    replaceAlg->setProperty<double>("NaNValue", 0.0);
    replaceAlg->setProperty<double>("NaNError", 0.0);
    replaceAlg->executeAsChildAlg();

    std::string outputWSName_frame = outputWSName + "_frame1_Iqxy";
    declareProperty(new WorkspaceProperty<>(
        "OutputWorkspaceFrame1", outputWSName_frame, Direction::Output));
    MatrixWorkspace_sptr result = replaceAlg->getProperty("OutputWorkspace");
    setProperty("OutputWorkspaceFrame1", result);

    // Frame 2
    params = Poco::NumberFormatter::format(wavelength_min_f2, 2) + ",0.1," +
             Poco::NumberFormatter::format(wavelength_max_f2, 2);
    rebinAlg = createChildAlgorithm("Rebin", 0.7, 0.8);
    rebinAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputWS);
    rebinAlg->setPropertyValue("Params", params);
    rebinAlg->setProperty("PreserveEvents", false);
    rebinAlg->executeAsChildAlg();

    qxyAlg = createChildAlgorithm("Qxy", .8, 0.95);
    qxyAlg->setProperty<MatrixWorkspace_sptr>(
        "InputWorkspace", rebinAlg->getProperty("OutputWorkspace"));
    qxyAlg->setProperty<double>("MaxQxy", qmax);
    qxyAlg->setProperty<double>("DeltaQ", qmax / nbins);
    qxyAlg->setProperty<bool>("SolidAngleWeighting", false);
    qxyAlg->executeAsChildAlg();

    qxy_output = qxyAlg->getProperty("OutputWorkspace");
    replaceAlg = createChildAlgorithm("ReplaceSpecialValues", .95, 1.0);
    replaceAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", qxy_output);
    replaceAlg->setProperty<double>("NaNValue", 0.0);
    replaceAlg->setProperty<double>("NaNError", 0.0);
    replaceAlg->executeAsChildAlg();

    outputWSName_frame = outputWSName + "_frame2_Iqxy";
    declareProperty(new WorkspaceProperty<>(
        "OutputWorkspaceFrame2", outputWSName_frame, Direction::Output));
    result = replaceAlg->getProperty("OutputWorkspace");
    setProperty("OutputWorkspaceFrame2", result);
    setProperty("OutputMessage", "I(Qx,Qy) computed for each frame");
  } else {
    // When not in frame skipping mode, simply run Qxy
    IAlgorithm_sptr qxyAlg = createChildAlgorithm("Qxy", .3, 0.9);
    qxyAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputWS);
    qxyAlg->setProperty<double>("MaxQxy", qmax);
    qxyAlg->setProperty<double>("DeltaQ", qmax / nbins);
    qxyAlg->setProperty<bool>("SolidAngleWeighting", false);
    qxyAlg->executeAsChildAlg();

    MatrixWorkspace_sptr qxy_output = qxyAlg->getProperty("OutputWorkspace");
    IAlgorithm_sptr replaceAlg =
        createChildAlgorithm("ReplaceSpecialValues", .9, 1.0);
    replaceAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", qxy_output);
    replaceAlg->setProperty<double>("NaNValue", 0.0);
    replaceAlg->setProperty<double>("NaNError", 0.0);
    replaceAlg->executeAsChildAlg();

    outputWSName += "_Iqxy";
    declareProperty(new WorkspaceProperty<>("OutputWorkspaceFrame1",
                                            outputWSName, Direction::Output));
    MatrixWorkspace_sptr result = replaceAlg->getProperty("OutputWorkspace");
    setProperty("OutputWorkspaceFrame1", result);
    setProperty("OutputMessage", "I(Qx,Qy) computed for each frame");
  }
}

} // namespace Algorithms
} // namespace Mantid
