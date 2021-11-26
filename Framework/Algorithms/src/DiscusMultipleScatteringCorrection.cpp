// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/DiscusMultipleScatteringCorrection.h"
#include "MantidAPI/ISpectrum.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/MersenneTwister.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using Mantid::DataObjects::Workspace2D;

namespace {
constexpr int DEFAULT_NPATHS = 1000;
constexpr int DEFAULT_SEED = 123456789;
constexpr int DEFAULT_NSCATTERINGS = 2;
constexpr int DEFAULT_LATITUDINAL_DETS = 5;
constexpr int DEFAULT_LONGITUDINAL_DETS = 10;
} // namespace

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(DiscusMultipleScatteringCorrection)

/**
 * Initialize the algorithm
 */
void DiscusMultipleScatteringCorrection::init() {
  // The input workspace must have an instrument and units of wavelength
  auto wsValidator = std::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<InstrumentValidator>();

  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input, wsValidator),
                  "The name of the input workspace.  The input workspace must "
                  "have X units of wavelength. This is used to supply the detector "
                  "positions and the x axis range to calculate corrections for");

  auto wsQValidator = std::make_shared<CompositeValidator>();
  wsQValidator->add<WorkspaceUnitValidator>("MomentumTransfer");

  declareProperty(std::make_unique<WorkspaceProperty<>>("SofqWorkspace", "", Direction::Input, wsQValidator),
                  "The name of the workspace containing S'(q).  The input "
                  "workspace must contain a single spectrum and "
                  "have X units of momentum transfer.");
  declareProperty(std::make_unique<WorkspaceProperty<WorkspaceGroup>>("OutputWorkspace", "", Direction::Output),
                  "Name for the WorkspaceGroup that will be created. Each workspace in the "
                  "group contains a calculated weight for a particular number of "
                  "scattering events. The number of scattering events varies from 1 up to "
                  "the number supplied in the NumberOfScatterings parameter. The group "
                  "will also include an additional workspace for a calculation with a "
                  "single scattering event where the absorption post scattering has been "
                  "set to zero");
  declareProperty(std::make_unique<WorkspaceProperty<>>("ScatteringCrossSection", "", Direction::Input,
                                                        PropertyMode::Optional, wsQValidator),
                  "A workspace containing the scattering cross section as a function of "
                  "k, :math:`\\sigma_s(k)`");

  auto positiveInt = std::make_shared<Kernel::BoundedValidator<int>>();
  positiveInt->setLower(1);
  declareProperty("NumberOfWavelengthPoints", EMPTY_INT(), positiveInt,
                  "The number of wavelength points for which a simulation is "
                  "attempted if ResimulateTracksForDifferentWavelengths=true");

  std::vector<std::string> propOptions{"Elastic", "Direct", "Indirect"};
  declareProperty("EMode", "Elastic", std::make_shared<Kernel::StringListValidator>(propOptions),
                  "The energy mode (default: Elastic)");

  declareProperty("NeutronPathsSingle", DEFAULT_NPATHS, positiveInt,
                  "The number of \"neutron\" paths to generate for single scattering");
  declareProperty("NeutronPathsMultiple", DEFAULT_NPATHS, positiveInt,
                  "The number of \"neutron\" paths to generate for multiple scattering");
  declareProperty("SeedValue", DEFAULT_SEED, positiveInt, "Seed the random number generator with this value");
  auto nScatteringsValidator = std::make_shared<Kernel::BoundedValidator<int>>();
  nScatteringsValidator->setLower(1);
  nScatteringsValidator->setUpper(5);
  declareProperty("NumberScatterings", DEFAULT_NSCATTERINGS, nScatteringsValidator, "Number of scatterings");

  auto interpolateOpt = createInterpolateOption();
  declareProperty(interpolateOpt->property(), interpolateOpt->propertyDoc());
  declareProperty("SparseInstrument", false,
                  "Enable simulation on special "
                  "instrument with a sparse grid of "
                  "detectors interpolating the "
                  "results to the real instrument.");
  auto threeOrMore = std::make_shared<Kernel::BoundedValidator<int>>();
  threeOrMore->setLower(3);
  declareProperty("NumberOfDetectorRows", DEFAULT_LATITUDINAL_DETS, threeOrMore,
                  "Number of detector rows in the detector grid of the sparse instrument.");
  setPropertySettings("NumberOfDetectorRows",
                      std::make_unique<EnabledWhenProperty>("SparseInstrument", ePropertyCriterion::IS_NOT_DEFAULT));
  auto twoOrMore = std::make_shared<Kernel::BoundedValidator<int>>();
  twoOrMore->setLower(2);
  declareProperty("NumberOfDetectorColumns", DEFAULT_LONGITUDINAL_DETS, twoOrMore,
                  "Number of detector columns in the detector grid "
                  "of the sparse instrument.");
  setPropertySettings("NumberOfDetectorColumns",
                      std::make_unique<EnabledWhenProperty>("SparseInstrument", ePropertyCriterion::IS_NOT_DEFAULT));
  declareProperty("ImportanceSampling", false,
                  "Enable importance sampling on the Q value chosen on multiple scatters based on Q.S(Q)");
  // Control the number of attempts made to generate a random point in the object
  declareProperty("MaxScatterPtAttempts", 5000, positiveInt,
                  "Maximum number of tries made to generate a scattering point "
                  "within the sample. Objects with holes in them, e.g. a thin "
                  "annulus can cause problems if this number is too low.\n"
                  "If a scattering point cannot be generated by increasing "
                  "this value then there is most likely a problem with "
                  "the sample geometry.");
}

/**
 * Validate the input properties.
 * @return a map where keys are property names and values the found issues
 */
std::map<std::string, std::string> DiscusMultipleScatteringCorrection::validateInputs() {
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  std::map<std::string, std::string> issues;
  Geometry::IComponent_const_sptr sample = inputWS->getInstrument()->getSample();
  if (!sample) {
    issues["InputWorkspace"] = "Input workspace does not have a Sample";
  } else {
    if (inputWS->sample().hasEnvironment()) {
      issues["InputWorkspace"] = "Sample must not have a sample environment";
    }

    if (inputWS->sample().getMaterial().numberDensity() == 0) {
      issues["InputWorkspace"] = "Sample must have a material set up with a non-zero number density";
    }
  }
  MatrixWorkspace_sptr SQWS = getProperty("SofqWorkspace");
  if (SQWS->getNumberHistograms() != 1) {
    issues["SofqWorkspace"] = "S(Q) workspace must contain a single spectrum";
  }
  auto y = SQWS->y(0);
  if (std::any_of(y.cbegin(), y.cend(), [](const auto yval) { return (yval < 0); })) {
    issues["SofqWorkspace"] = "S(Q) workspace must have all y >= 0";
  }

  const std::string emodeStr = getProperty("EMode");
  auto emode = Kernel::DeltaEMode::fromString(emodeStr);
  if (emode != Kernel::DeltaEMode::Elastic) {
    issues["EMode"] = "Only elastic mode is supported at the moment";
  }

  const int nlambda = getProperty("NumberOfWavelengthPoints");
  if (!isEmpty(nlambda)) {
    InterpolationOption interpOpt;
    const std::string interpValue = getPropertyValue("Interpolation");
    interpOpt.set(interpValue, false, false);
    const auto nlambdaIssue = interpOpt.validateInputSize(nlambda);
    if (!nlambdaIssue.empty()) {
      issues["NumberOfWavelengthPoints"] = nlambdaIssue;
    }
  }

  return issues;
}

/**
 * Execution code
 */
void DiscusMultipleScatteringCorrection::exec() {
  g_log.warning(
      "DiscusMultipleScatteringCorrection is in the beta stage of development. Its name, properties and behaviour "
      "may change without warning.");
  if (!getAlwaysStoreInADS())
    throw std::runtime_error("This algorithm explicitly stores named output workspaces in the ADS so must be run with "
                             "AlwaysStoreInADS set to true");
  const MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr SQWS = getProperty("SofqWorkspace");
  // avoid repeated conversion of bin edges to points inside loop by converting to point data
  if (SQWS->isHistogramData()) {
    auto pointDataAlgorithm = this->createChildAlgorithm("ConvertToPointData");
    pointDataAlgorithm->initialize();
    pointDataAlgorithm->setProperty("InputWorkspace", SQWS);
    pointDataAlgorithm->setProperty("OutputWorkspace", "_");
    pointDataAlgorithm->execute();
    SQWS = pointDataAlgorithm->getProperty("OutputWorkspace");
  }

  MatrixWorkspace_sptr sigmaSSWS = getProperty("ScatteringCrossSection");
  if (sigmaSSWS)
    m_sigmaSS = std::make_shared<HistogramData::Histogram>(sigmaSSWS->histogram(0));

  double lambdamin, lambdamax;
  inputWS->getXMinMax(lambdamin, lambdamax);
  m_SQHist = std::make_shared<HistogramData::Histogram>(SQWS->histogram(0));
  auto QSQHist = prepareQSQ(SQHist, 2 * M_PI / lambdamin);

  const auto inputNbins = static_cast<int>(inputWS->blocksize());
  int nlambda = getProperty("NumberOfWavelengthPoints");
  if (isEmpty(nlambda) || nlambda > inputNbins) {
    if (!isEmpty(nlambda)) {
      g_log.warning() << "The requested number of wavelength points is larger "
                         "than the spectra size. "
                         "Defaulting to spectra size.\n";
    }
    nlambda = inputNbins;
  }

  const bool useSparseInstrument = getProperty("SparseInstrument");
  SparseWorkspace_sptr sparseWS;
  if (useSparseInstrument) {
    const int latitudinalDets = getProperty("NumberOfDetectorRows");
    const int longitudinalDets = getProperty("NumberOfDetectorColumns");
    sparseWS = createSparseWorkspace(*inputWS, nlambda, latitudinalDets, longitudinalDets);
  }
  const int nScatters = getProperty("NumberScatterings");
  m_maxScatterPtAttempts = getProperty("MaxScatterPtAttempts");
  std::vector<MatrixWorkspace_sptr> simulationWSs;
  std::vector<MatrixWorkspace_sptr> outputWSs;

  auto noAbsOutputWS = createOutputWorkspace(*inputWS);
  auto noAbsSimulationWS = useSparseInstrument ? sparseWS->clone() : noAbsOutputWS;
  for (int i = 0; i < nScatters; i++) {
    auto outputWS = createOutputWorkspace(*inputWS);
    MatrixWorkspace_sptr simulationWS = useSparseInstrument ? sparseWS->clone() : outputWS;
    simulationWSs.push_back(simulationWS);
    outputWSs.push_back(outputWS);
  }
  const MatrixWorkspace &instrumentWS = useSparseInstrument ? *sparseWS : *inputWS;

  m_instrument = inputWS->getInstrument();
  const auto nhists = useSparseInstrument ? static_cast<int64_t>(sparseWS->getNumberHistograms())
                                          : static_cast<int64_t>(inputWS->getNumberHistograms());

  m_sampleShape = inputWS->sample().getShapePtr();
  // generate the bounding box before the multithreaded section
  m_sampleShape->getBoundingBox();

  const int nSingleScatterEvents = getProperty("NeutronPathsSingle");
  const int nMultiScatterEvents = getProperty("NeutronPathsMultiple");

  const int seed = getProperty("SeedValue");

  InterpolationOption interpolateOpt;
  interpolateOpt.set(getPropertyValue("Interpolation"), false, true);

  m_importanceSampling = getProperty("ImportanceSampling");

  Progress prog(this, 0.0, 1.0, nhists * nlambda);
  prog.setNotifyStep(0.01);
  const std::string reportMsg = "Computing corrections";

  bool enableParallelFor = true;
  enableParallelFor = std::all_of(simulationWSs.cbegin(), simulationWSs.cend(),
                                  [](const MatrixWorkspace_sptr &ws) { return Kernel::threadSafe(*ws); });

  enableParallelFor = enableParallelFor && Kernel::threadSafe(*noAbsOutputWS);

  const auto &spectrumInfo = instrumentWS.spectrumInfo();

  PARALLEL_FOR_IF(enableParallelFor)
  for (int64_t i = 0; i < nhists; ++i) {
    PARALLEL_START_INTERUPT_REGION
    auto &spectrum = instrumentWS.getSpectrum(i);
    Mantid::specnum_t specNo = spectrum.getSpectrumNo();
    MersenneTwister rng(seed + specNo);
    // no two theta for monitors

    if (spectrumInfo.hasDetectors(i) && !spectrumInfo.isMonitor(i) && !spectrumInfo.isMasked(i)) {

      const auto lambdas = instrumentWS.points(i).rawData();

      const auto nbins = lambdas.size();
      // step size = index range / number of steps requested
      const size_t nsteps = std::max(1, nlambda - 1);
      const size_t lambdaStepSize = nbins == 1 ? 1 : (nbins - 1) / nsteps;

      const auto detPos = spectrumInfo.position(i);

      for (size_t bin = 0; bin < nbins; bin += lambdaStepSize) {

        const double kinc = 2 * M_PI / lambdas[bin];

        auto invPOfQ = prepareCumulativeProbForQ(*QSQHist, kinc);

        double total = simulatePaths(nSingleScatterEvents, 1, rng, *invPOfQ, kinc, detPos, true);
        noAbsSimulationWS->getSpectrum(i).dataY()[bin] = total;

        for (int ne = 0; ne < nScatters; ne++) {
          int nEvents = ne == 0 ? nSingleScatterEvents : nMultiScatterEvents;

          total = simulatePaths(nEvents, ne + 1, rng, *invPOfQ, kinc, detPos, false);
          simulationWSs[ne]->getSpectrum(i).dataY()[bin] = total;
        }

        prog.report(reportMsg);

        // Ensure we have the last point for the interpolation
        if (lambdaStepSize > 1 && bin + lambdaStepSize >= nbins && bin + 1 != nbins) {
          bin = nbins - lambdaStepSize - 1;
        }
      } // bins

      // interpolate through points not simulated. Simulation WS only has
      // reduced X values if using sparse instrument so no interpolation
      // required
      if (!useSparseInstrument && lambdaStepSize > 1) {
        auto histNoAbs = noAbsSimulationWS->histogram(i);
        if (lambdaStepSize < nbins) {
          interpolateOpt.applyInplace(histNoAbs, lambdaStepSize);
        } else {
          std::fill(histNoAbs.mutableY().begin() + 1, histNoAbs.mutableY().end(), histNoAbs.y()[0]);
        }
        noAbsOutputWS->setHistogram(i, histNoAbs);

        for (size_t ne = 0; ne < static_cast<size_t>(nScatters); ne++) {
          auto histnew = simulationWSs[ne]->histogram(i);
          if (lambdaStepSize < nbins) {
            interpolateOpt.applyInplace(histnew, lambdaStepSize);
          } else {
            std::fill(histnew.mutableY().begin() + 1, histnew.mutableY().end(), histnew.y()[0]);
          }
          outputWSs[ne]->setHistogram(i, histnew);
        }
      }
    }

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  if (useSparseInstrument) {
    Poco::Thread::sleep(200); // to ensure prog message changes
    const std::string reportMsgSpatialInterpolation = "Spatial Interpolation";
    prog.report(reportMsgSpatialInterpolation);
    interpolateFromSparse(*noAbsOutputWS, *std::dynamic_pointer_cast<SparseWorkspace>(noAbsSimulationWS),
                          interpolateOpt);
    for (size_t ne = 0; ne < static_cast<size_t>(nScatters); ne++) {
      interpolateFromSparse(*outputWSs[ne], *std::dynamic_pointer_cast<SparseWorkspace>(simulationWSs[ne]),
                            interpolateOpt);
    }
  }

  // Create workspace group that holds output workspaces
  auto wsgroup = std::make_shared<WorkspaceGroup>();
  auto outputGroupWSName = getPropertyValue("OutputWorkspace");
  if (AnalysisDataService::Instance().doesExist(outputGroupWSName))
    API::AnalysisDataService::Instance().deepRemoveGroup(outputGroupWSName);

  const std::string wsNamePrefix = "Scatter_";
  std::string wsName = wsNamePrefix + "1_NoAbs";
  setWorkspaceName(noAbsOutputWS, wsName);
  wsgroup->addWorkspace(noAbsOutputWS);

  for (size_t i = 0; i < outputWSs.size(); i++) {
    wsName = wsNamePrefix + std::to_string(i + 1);
    setWorkspaceName(outputWSs[i], wsName);
    wsgroup->addWorkspace(outputWSs[i]);
  }

  if (outputWSs.size() > 1) {
    auto summedOutput = createOutputWorkspace(*inputWS);
    for (size_t i = 1; i < outputWSs.size(); i++) {
      summedOutput = summedOutput + outputWSs[i];
    }
    wsName = "Scatter_2_" + std::to_string(outputWSs.size()) + "_Summed";
    setWorkspaceName(summedOutput, wsName);
    wsgroup->addWorkspace(summedOutput);
  }

  // set the output property
  setProperty("OutputWorkspace", wsgroup);

  if (g_log.is(Kernel::Logger::Priority::PRIO_WARNING)) {
    for (auto &kv : m_attemptsToGenerateInitialTrack)
      g_log.warning() << "Generating initial track required " + std::to_string(kv.first) + " attempts on " +
                             std::to_string(kv.second) + " occasions.\n";
    g_log.warning() << "Calls to interceptSurface= " + std::to_string(m_callsToInterceptSurface) + "\n";
  }
}

std::unique_ptr<Mantid::HistogramData::Histogram>
DiscusMultipleScatteringCorrection::prepareQSQ(HistogramData::Histogram &SQ, double kmax) {
  std::vector<double> qValues = SQ.readX();
  std::vector<double> SQValues = SQ.readY();
  // add terminating points at 0 and 2k before multiplying by Q so no extrapolation problems
  if (qValues.front() > 0.) {
    qValues.insert(qValues.begin(), 0.);
    SQValues.insert(SQValues.begin(), SQValues.front());
  }
  if (qValues.back() < 2 * kmax) {
    qValues.push_back(2 * kmax);
    SQValues.push_back(SQValues.back());
  }
  // add some extra points to help the Q.S(Q) integral get the right answer
  qValues.reserve(qValues.size() * 2);
  SQValues.reserve(SQValues.size() * 2);
  for (size_t i = 1; i < qValues.size(); i++) {
    if (std::abs(SQValues[i] - SQValues[i - 1]) > std::numeric_limits<double>::epsilon()) {
      qValues.insert(qValues.begin() + i, qValues[i] - std::numeric_limits<double>::epsilon());
      SQValues.insert(SQValues.begin() + i, SQValues[i - 1]);
      i++;
    }
  }
  std::vector<double> QSQValues;
  std::transform(SQValues.begin(), SQValues.end(), qValues.begin(), std::back_inserter(QSQValues),
                 std::multiplies<double>());
  return std::make_unique<Mantid::HistogramData::Histogram>(Mantid::HistogramData::Points{qValues},
                                                            Mantid::HistogramData::Counts{QSQValues});
}

/**
 * Calculate a cumulative probability distribution for use in importance sampling. The distribution
 * is the inverse function P^-1(t4) where P(Q) = I(Q)/I(2k) and I(x) = integral of Q.S(Q)dQ between 0 and x
 * @param SQ Workspace containing S(Q)
 * @param kinc The incident wavenumber
 * @return the inverted cumulative probability distribution
 */
std::unique_ptr<Mantid::HistogramData::Histogram>
DiscusMultipleScatteringCorrection::prepareCumulativeProbForQ(HistogramData::Histogram &QSQ, double kinc) {

  auto IOfQ = integrateCumulative(QSQ, 2 * kinc);
  auto IOfQX = IOfQ->dataX();
  auto IOfQY = IOfQ->dataY();
  auto IOfQYAt2K = IOfQY.back();
  if (IOfQYAt2K == 0.)
    throw std::runtime_error("Integral of Q * S(Q) is zero so can't generate probability distribution");
  std::transform(IOfQY.begin(), IOfQY.end(), IOfQY.begin(), [IOfQYAt2K](double d) -> double { return d / IOfQYAt2K; });
  // return std::make_shared<Mantid::HistogramData::Histogram>(IOfQ->y(), IOfQ->x());
  return std::make_unique<Mantid::HistogramData::Histogram>(
      Mantid::HistogramData::Points{std::move(IOfQY)}, Mantid::HistogramData::Counts{std::move(IOfQX)},
      Mantid::HistogramData::CountStandardDeviations(IOfQY.size(), 0));
}

/**
 * Integrate a distribution between x=0 and the supplied xmax value using trapezoid rule
 * without any extrapolation on either end of the distribution
 * @param h Histogram object containing the distribution to integrate
 * @param xmax The upper integration limit
 * @return A histogram containing the integral values for each point in the supplied histogram
 */
std::unique_ptr<Mantid::HistogramData::Histogram>
DiscusMultipleScatteringCorrection::integrateCumulative(const Mantid::HistogramData::Histogram &h, double xmax) {
  const std::vector<double> &xValues = h.readX();
  const std::vector<double> &yValues = h.readY();
  // don't need the e storage in the histogram so use this constructor for speed
  auto result = std::make_unique<Mantid::HistogramData::Histogram>(HistogramData::Histogram::XMode::Points,
                                                                   HistogramData::Histogram::YMode::Counts);
  result->setY(Mantid::Kernel::make_cow<HistogramData::HistogramY>(0));
  auto &resultX = result->dataX();
  auto &resultY = result->dataY();
  resultX.reserve(xValues.size());
  resultY.reserve(yValues.size());
  // set the integral to zero at x=0
  resultX.push_back(0.);
  resultY.push_back(0.);
  double sum = 0;

  // ensure there's a point at x=0 so xmax is never to the left of all the points
  if (xValues.front() > 0.)
    throw std::runtime_error("Distribution doesn't contain a point at x=0");
  // ...and a terminating point. Q.S(Q) generally not flat so assuming flat extrapolation not v useful
  if (xValues.back() < xmax)
    throw std::runtime_error("Distribution doesn't extend as far as upper integration limit, x=" +
                             std::to_string(xmax));

  size_t iRight;
  // integrate the intervals between each pair of points. Do this until right point is at end of vector or > xmax
  for (iRight = 1; iRight < xValues.size() && xValues[iRight] <= xmax; iRight++) {
    sum += 0.5 * (yValues[iRight] + yValues[iRight - 1]) * (xValues[iRight] - xValues[iRight - 1]);
    resultX.push_back(xValues[iRight]);
    resultY.push_back(sum);
  }

  // integrate a partial final interval if xmax is between points
  if (xmax > xValues[iRight - 1]) {
    // use linear interpolation to calculate the y value at xmax
    double interpY = (yValues[iRight - 1] * (xValues[iRight] - xmax) + yValues[iRight] * (xmax - xValues[iRight - 1])) /
                     (xValues[iRight] - xValues[iRight - 1]);
    sum += 0.5 * (yValues[iRight - 1] + interpY) * (xmax - xValues[iRight - 1]);
    resultX.push_back(xmax);
    resultY.push_back(sum);
  }
  return result;
}

/**
 * Calculate a total cross section using a k-specific scattering cross section
 * Note - a separate tabulated scattering cross section is used elsewhere in the
 * calculation
 * @param material The sample material
 * @param kinc The incident wavenumber
 * @param specialSingleScatterCalc Boolean indicating whether special single
 * scatter calculation should be performed
 * @return A tuple containing the total cross section and the scattering cross section
 */
std::tuple<double, double> DiscusMultipleScatteringCorrection::new_vector(const Material &material, double kinc,
                                                                          bool specialSingleScatterCalc) {
  double scatteringXSection, absorbXsection;
  if (specialSingleScatterCalc) {
    absorbXsection = 0;
  } else {
    const double wavelength = 2 * M_PI / kinc;
    absorbXsection = material.absorbXSection(wavelength);
  }
  if (m_sigmaSS) {
    scatteringXSection = interpolateFlat(m_sigmaSS, kinc);
  } else {
    scatteringXSection = material.totalScatterXSection();
  }

  const auto sig_total = scatteringXSection + absorbXsection;
  return {sig_total, scatteringXSection};
}

/**
 * Interpolate function of the form y = a * sqrt(x - b) ie inverse of a quadratic
 * Used to lookup value in the cumulative probability distribution of Q S(Q) which
 * for flat S(Q) will be a quadratic
 */
double DiscusMultipleScatteringCorrection::interpolateSquareRoot(const HistogramData::Histogram &histToInterpolate,
                                                                 double x) {
  assert(histToInterpolate.xMode() == HistogramData::Histogram::XMode::Points);
  if (x > histToInterpolate.x().back()) {
    return histToInterpolate.y().back();
  }
  if (x < histToInterpolate.x().front()) {
    return histToInterpolate.y().front();
  }
  auto iter = std::upper_bound(histToInterpolate.x().cbegin(), histToInterpolate.x().cend(), x);
  auto idx = static_cast<size_t>(std::distance(histToInterpolate.x().cbegin(), iter) - 1);
  const auto &y = histToInterpolate.y();
  double x0 = histToInterpolate.x()[idx];
  double x1 = histToInterpolate.x()[idx + 1];
  double a = sqrt((pow(y[idx + 1], 2) - pow(y[idx], 2)) / (x1 - x0));
  if (a == 0.) {
    throw std::runtime_error("Cannot perform square root interpolation on supplied distribution");
  }
  double b = x0 - pow(y[idx], 2) / pow(a, 2);
  return a * sqrt(x - b);
}

/**
 * Interpolate function using flat interpolation from previous point
 */
double DiscusMultipleScatteringCorrection::interpolateFlat(
    std::shared_ptr<const Mantid::HistogramData::Histogram> histToInterpolate, double x) {
  auto &xHisto = histToInterpolate->x();
  auto &yHisto = histToInterpolate->y();
  if (x > xHisto.back()) {
    return yHisto.back();
  }
  if (x < xHisto.front()) {
    return yHisto.front();
  }
  auto iter = std::upper_bound(xHisto.cbegin(), xHisto.cend(), x);
  auto idx = static_cast<size_t>(std::distance(xHisto.cbegin(), iter) - 1);
  return yHisto[idx];
}

/**
 * Simulates a set of neutron paths through the sample to a specific detector
 * position with each path containing the specified number of scattering events.
 * Each path represents a group of neutrons and the proportion of neutrons
 * making it to the destination without being scattered or absorbed is
 * calculated as a weight using the cross section information from the sample
 * material. The average weight across all the simulated paths is returned
 * @param nPaths The number of paths to simulate
 * @param nScatters The number of scattering events to simulate along each path
 * @param rng Random number generator
 * @param invPOfQ Inverse of the cumulative prob distribution of Q (used in importance sampling)
 * @param kinc The incident wavevector
 * @param detPos The position of the detector we're currently calculating a correction for
 * @param specialSingleScatterCalc Boolean indicating whether special single
 * @return An average weight across all of the paths
 */
double DiscusMultipleScatteringCorrection::simulatePaths(const int nPaths, const int nScatters,
                                                         Kernel::PseudoRandomNumberGenerator &rng,
                                                         const HistogramData::Histogram &invPOfQ, const double kinc,
                                                         const Kernel::V3D &detPos, bool specialSingleScatterCalc) {
  double sumOfWeights = 0, sumOfQSS = 0.;
  int countZeroWeights = 0; // for debugging and analysis of where importance sampling may help

  for (int ie = 0; ie < nPaths; ie++) {
    auto [success, weight, QSS] = scatter(nScatters, rng, invPOfQ, kinc, detPos, specialSingleScatterCalc);
    if (success) {
      sumOfWeights += weight;
      sumOfQSS += QSS;
      if (weight == 0.)
        countZeroWeights++;
    } else
      ie--;
  }

  // divide by the mean of Q*S(Q) for each of the n-1 terms representing a
  // multiple scatter
  if (!m_importanceSampling)
    sumOfWeights = sumOfWeights / pow(sumOfQSS / static_cast<double>(nPaths * (nScatters - 1)), nScatters - 1);

  return sumOfWeights / nPaths;
}

/**
 * Simulates a single neutron path through the sample to a specific detector
 * position containing the specified number of scattering events.
 * Each path represents a group of neutrons and the proportion of neutrons
 * making it to the destination without being scattered or absorbed is
 * calculated as a weight using the cross section information from the sample
 * material
 * @param nScatters The number of scattering events to simulate along each path
 * @param rng Random number generator
 * @param invPOfQ Inverse of the cumulative prob distribution of Q (used in importance sampling)
 * @param kinc The incident wavevector
 * @param detPos The detector position xyz coordinates
 * @param specialSingleScatterCalc Boolean indicating whether special single
 * scatter calculation should be performed
 * @return A tuple containing a success/fail boolean, the calculated weight and
 * a sum of the QSS values across the n-1 multiple scatters
 */
std::tuple<bool, double, double>
DiscusMultipleScatteringCorrection::scatter(const int nScatters, Kernel::PseudoRandomNumberGenerator &rng,
                                            const HistogramData::Histogram &invPOfQ, const double kinc,
                                            const Kernel::V3D &detPos, bool specialSingleScatterCalc) {
  double weight = 1;
  double numberDensity = m_sampleShape->material().numberDensityEffective();
  // if scale up scatteringXSection by 100*numberDensity then may not need
  // sigma_total any more but leave it alone now to align with original code

  const auto [sigma_total, scatteringXSection] = new_vector(m_sampleShape->material(), kinc, specialSingleScatterCalc);

  double vmu = 100 * numberDensity * sigma_total;
  auto track = start_point(rng);
  updateWeightAndPosition(track, weight, vmu, sigma_total, rng);

  double QSS = 0;
  for (int iScat = 0; iScat < nScatters - 1; iScat++) {
    q_dir(track, invPOfQ, kinc, scatteringXSection, rng, QSS, weight);
    const int nlinks = m_sampleShape->interceptSurface(track);
    m_callsToInterceptSurface++;
    if (nlinks == 0) {
      return {false, 0, 0};
    }
    updateWeightAndPosition(track, weight, vmu, sigma_total, rng);
  }

  Kernel::V3D directionToDetector = detPos - track.startPoint();
  Kernel::V3D prevDirection = track.direction();
  directionToDetector.normalize();
  track.reset(track.startPoint(), directionToDetector);
  const int nlinks = m_sampleShape->interceptSurface(track);
  m_callsToInterceptSurface++;
  // due to VALID_INTERCEPT_POINT_SHIFT some tracks that skim the surface
  // of a CSGObject sample may not generate valid tracks. Start over again
  // for this event
  if (nlinks == 0) {
    return {false, 0, 0};
  }
  const double dl = track.front().distInsideObject;
  const auto q = (directionToDetector - prevDirection) * kinc;
  const auto SQ = interpolateFlat(m_SQHist, q.norm());
  if (specialSingleScatterCalc)
    vmu = 0;
  const auto AT2 = exp(-dl * vmu);
  auto scatteringXSectionFull = m_sampleShape->material().totalScatterXSection();
  weight = weight * AT2 * SQ * scatteringXSectionFull / (4 * M_PI);
  return {true, weight, QSS};
}

// update track direction, QSS and weight
void DiscusMultipleScatteringCorrection::q_dir(Geometry::Track &track, const HistogramData::Histogram &invPOfQ,
                                               const double kinc, const double scatteringXSection,
                                               Kernel::PseudoRandomNumberGenerator &rng, double &QSS, double &weight) {
  const double kf = kinc; // elastic only so far
  double QQ, SQ;
  if (m_importanceSampling) {
    QQ = interpolateSquareRoot(invPOfQ, rng.nextValue());
    // S(Q) not strictly needed here but useful to see if the higher values are indeed being returned
    SQ = interpolateFlat(m_SQHist, QQ);
    weight = weight * scatteringXSection;
  } else {
    const double qmin = abs(kf - kinc);
    const double qrange = 2 * kinc;
    QQ = qmin + qrange * rng.nextValue();
    SQ = interpolateFlat(m_SQHist, QQ);
    weight = weight * scatteringXSection * SQ * QQ;
  }
  // T = 2theta
  const double cosT = (kinc * kinc + kf * kf - QQ * QQ) / (2 * kinc * kf);

  QSS += QQ * SQ;

  updateTrackDirection(track, cosT, rng.nextValue() * 2 * M_PI);
}

/**
 * Update the track's direction following a scatter event given theta and phi angles
 * @param track The track whose direction will be updated
 * @param cosT Cos two theta. two theta is scattering angle
 * @param phi Phi (radians) of after track. Measured in plane perpendicular to initial trajectory
 */
void DiscusMultipleScatteringCorrection::updateTrackDirection(Geometry::Track &track, const double cosT,
                                                              const double phi) {
  const auto B3 = sqrt(1 - cosT * cosT);
  const auto B2 = cosT;
  // possible to do this using the Quat class instead??
  // Quat(const double _deg, const V3D &_axis);
  // Quat(acos(cosT)*180/M_PI,
  // Kernel::V3D(track.direction()[],track.direction()[],0))

  // Rodrigues formula with final term equal to zero
  // v_rot = cosT * v + sinT(k x v)
  // with rotation axis k orthogonal to v
  // Define k by first creating two vectors orthogonal to v:
  // (vy, -vx, 0) by inspection
  // and then (-vz * vx, -vy * vz, vx * vx + vy * vy) as cross product
  // Then define k as combination of these:
  // sin(phi) * (vy, -vx, 0) + cos(phi) * (-vx * vz, -vy * vz, 1 - vz * vz)
  // ...with division by normalisation factor of sqrt(vx * vx + vy * vy)
  // Note: xyz convention here isn't the standard Mantid one. x=beam, z=up
  const auto vy = track.direction()[0];
  const auto vz = track.direction()[1];
  const auto vx = track.direction()[2];
  double UKX, UKY, UKZ;
  if (vz * vz < 1.0) {
    // calculate A2 from vx^2 + vy^2 rather than 1-vz^2 to reduce floating point rounding error when vz close to 1
    auto A2 = sqrt(vx * vx + vy * vy);
    auto UQTZ = cos(phi) * A2;
    auto UQTX = -cos(phi) * vz * vx / A2 + sin(phi) * vy / A2;
    auto UQTY = -cos(phi) * vz * vy / A2 - sin(phi) * vx / A2;
    UKX = B2 * vx + B3 * UQTX;
    UKY = B2 * vy + B3 * UQTY;
    UKZ = B2 * vz + B3 * UQTZ;
  } else {
    // definition of phi in general formula is dependent on v. So may see phi "redefinition" as vx and vy tend to zero
    // and you move from general formula to this special case
    UKX = B3 * cos(phi);
    UKY = B3 * sin(phi);
    UKZ = B2 * vz;
  }
  track.reset(track.startPoint(), Kernel::V3D(UKY, UKZ, UKX));
}

/**
 * Repeatedly attempt to generate an initial track starting at the source and entering the sample at a random point on
 * its front surface. After each attempt check the track has at least one intercept with sample shape (sometimes for
 * tracks very close to the surface this can sometimes be zero due to numerical precision)
 * @param rng Random number generator
 * @return a track intercepting the sample
 */
Geometry::Track DiscusMultipleScatteringCorrection::start_point(Kernel::PseudoRandomNumberGenerator &rng) {
  for (int i = 0; i < m_maxScatterPtAttempts; i++) {
    auto t = generateInitialTrack(rng);
    const int nlinks = m_sampleShape->interceptSurface(t);
    m_callsToInterceptSurface++;
    if (nlinks > 0) {
      if (i > 0) {
        if (g_log.is(Kernel::Logger::Priority::PRIO_WARNING)) {
          m_attemptsToGenerateInitialTrack[i + 1]++;
        }
      }
      return t;
    }
  }
  throw std::runtime_error(
      "DiscusMultipleScatteringCorrection::start_point() - Unable to generate entry point into sample after " +
      std::to_string(m_maxScatterPtAttempts) + " attempts. Try increasing MaxScatterPtAttempts");
}

/** update track start point and weight. The weight is based on a change of variables from length to t1
 * as described in Mancinelli paper
 * @param track A track defining the current trajectory
 * @param weight The weight for the current path that is about to be updated
 * @param vmu The total attenuation coefficient
 * @param sigma_total The total cross section (scattering + absorption)
 * @param rng Random number generator
 */

void DiscusMultipleScatteringCorrection::updateWeightAndPosition(Geometry::Track &track, double &weight,
                                                                 const double vmu, const double sigma_total,
                                                                 Kernel::PseudoRandomNumberGenerator &rng) {
  // work out maximum distance to next scatter point dl
  // At the moment this doesn't cope if sample shape is concave eg if track has more than one segment inside the
  // sample with segment outside sample in between
  const double dl = track.front().distInsideObject;
  const double b4 = (1.0 - exp(-dl * vmu));
  const double vmfp = 1.0 / vmu;
  const double vl = -(vmfp * log(1 - rng.nextValue() * b4));
  weight = weight * b4 / sigma_total;
  inc_xyz(track, vl);
}

/**
 * Generate an initial track starting at the source and entering
 * the sample at a random point on its front surface
 * @param rng Random number generator
 * @return a track
 */
Geometry::Track DiscusMultipleScatteringCorrection::generateInitialTrack(Kernel::PseudoRandomNumberGenerator &rng) {
  auto sampleBox = m_sampleShape->getBoundingBox();
  // generate random point on front surface of sample bounding box
  // The change of variables from length to t1 means this still samples the points fairly in the integration volume
  // even in shapes like cylinders where the depth varies across xy
  auto frame = m_instrument->getReferenceFrame();
  auto sourcePos = m_instrument->getSource()->getPos();
  auto ptx = sampleBox.minPoint()[frame->pointingHorizontal()] +
             rng.nextValue() * sampleBox.width()[frame->pointingHorizontal()];
  auto pty = sampleBox.minPoint()[frame->pointingUp()] + rng.nextValue() * sampleBox.width()[frame->pointingUp()];

  // perhaps eventually also generate random point on the beam profile?
  auto ptOnBeamProfile = Kernel::V3D();
  ptOnBeamProfile[frame->pointingHorizontal()] = ptx;
  ptOnBeamProfile[frame->pointingUp()] = pty;
  ptOnBeamProfile[frame->pointingAlongBeam()] = sourcePos[frame->pointingAlongBeam()];
  auto toSample = Kernel::V3D();
  toSample[frame->pointingAlongBeam()] = 1.;
  return Geometry::Track(ptOnBeamProfile, toSample);
}

/**
 * Update the x, y, z position of the neutron (or dV volume element
 * to integrate over). Save new start point in to the track object
 * supplied along
 * @param track A track defining the current trajectory
 * @param vl A distance to move along the current trajectory
 */
void DiscusMultipleScatteringCorrection::inc_xyz(Geometry::Track &track, double vl) {
  Kernel::V3D position = track.front().entryPoint;
  Kernel::V3D direction = track.direction();
  const auto x = position[0] + vl * direction[0];
  const auto y = position[1] + vl * direction[1];
  const auto z = position[2] + vl * direction[2];
  const auto startPoint = V3D(x, y, z);
  track.clearIntersectionResults();
  track.reset(startPoint, track.direction());
}

/**
 * Factory method to return an instance of the required SparseInstrument class
 * @param modelWS The full workspace that the sparse one will be based on
 * @param wavelengthPoints The number of wavelength points to include in the
 * histograms in the sparse workspace
 * @param rows The number of rows of detectors to create
 * @param columns The number of columns of detectors to create
 * @return a pointer to an SparseInstrument object
 */
std::shared_ptr<SparseWorkspace> DiscusMultipleScatteringCorrection::createSparseWorkspace(
    const API::MatrixWorkspace &modelWS, const size_t wavelengthPoints, const size_t rows, const size_t columns) {
  auto sparseWS = std::make_shared<SparseWorkspace>(modelWS, wavelengthPoints, rows, columns);
  return sparseWS;
}

MatrixWorkspace_sptr DiscusMultipleScatteringCorrection::createOutputWorkspace(const MatrixWorkspace &inputWS) const {
  MatrixWorkspace_uptr outputWS = DataObjects::create<Workspace2D>(inputWS);
  // The algorithm computes the signal values at bin centres so they should
  // be treated as a distribution
  outputWS->setDistribution(true);
  outputWS->setYUnit("");
  outputWS->setYUnitLabel("Scattered Weight");
  return outputWS;
}

/**
 * Factory method to return an instance of the required InterpolationOption
 * class
 * @return a pointer to an InterpolationOption object
 */
std::unique_ptr<InterpolationOption> DiscusMultipleScatteringCorrection::createInterpolateOption() {
  auto interpolationOpt = std::make_unique<InterpolationOption>();
  return interpolationOpt;
}

void DiscusMultipleScatteringCorrection::interpolateFromSparse(
    MatrixWorkspace &targetWS, const SparseWorkspace &sparseWS,
    const Mantid::Algorithms::InterpolationOption &interpOpt) {
  const auto &spectrumInfo = targetWS.spectrumInfo();
  const auto refFrame = targetWS.getInstrument()->getReferenceFrame();
  PARALLEL_FOR_IF(Kernel::threadSafe(targetWS, sparseWS))
  for (int64_t i = 0; i < static_cast<decltype(i)>(spectrumInfo.size()); ++i) {
    PARALLEL_START_INTERUPT_REGION
    if (spectrumInfo.hasDetectors(i) && !spectrumInfo.isMonitor(i)) {
      double lat, lon;
      std::tie(lat, lon) = spectrumInfo.geographicalAngles(i);
      const auto spatiallyInterpHisto = sparseWS.bilinearInterpolateFromDetectorGrid(lat, lon);
      if (spatiallyInterpHisto.size() > 1) {
        auto targetHisto = targetWS.histogram(i);
        interpOpt.applyInPlace(spatiallyInterpHisto, targetHisto);
        targetWS.setHistogram(i, targetHisto);
      } else {
        targetWS.mutableY(i) = spatiallyInterpHisto.y().front();
      }
    }
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
}

void DiscusMultipleScatteringCorrection::correctForWorkspaceNameClash(std::string &wsName) {
  bool noClash(false);

  for (int i = 0; !noClash; ++i) {
    std::string wsIndex; // dont use an index if there is no other
                         // workspace
    if (i > 0) {
      wsIndex = "_" + std::to_string(i);
    }

    bool wsExists = AnalysisDataService::Instance().doesExist(wsName + wsIndex);
    if (!wsExists) {
      wsName += wsIndex;
      noClash = true;
    }
  }
}

/**
 * Set the name on a workspace, adjusting for potential clashes in the ADS.
 * Used to set the names on the output workspace group members. N
 * @param ws The ws to set the name on
 * @param wsName The name to set on the workspace
 */
void DiscusMultipleScatteringCorrection::setWorkspaceName(const API::MatrixWorkspace_sptr &ws, std::string wsName) {
  correctForWorkspaceNameClash(wsName);
  API::AnalysisDataService::Instance().addOrReplace(wsName, ws);
}

} // namespace Mantid::Algorithms
