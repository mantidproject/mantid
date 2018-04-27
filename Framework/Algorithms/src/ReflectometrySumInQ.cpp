#include "MantidAlgorithms/ReflectometrySumInQ.h"

#include "MantidAPI/Algorithm.tcc"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/IDetector.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidIndexing/SpectrumNumber.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/Strings.h"

namespace {
namespace Prop {
const static std::string BEAM_CENTRE{"BeamCentre"};
const static std::string INPUT_WS{"InputWorkspace"};
const static std::string IS_FLAT_SAMPLE{"FlatSample"};
const static std::string OUTPUT_WS{"OutputWorkspace"};
const static std::string WAVELENGTH_MAX{"WavelengthMax"};
const static std::string WAVELENGTH_MIN{"WavelengthMin"};
}

/**
 * Share the given input counts into the output array bins proportionally
 * according to how much the bins overlap the given lambda range.
 * outputX.size() must equal outputY.size() + 1
 *
 * @param inputCounts [in] :: the input counts to share out
 * @param inputErr [in] :: the input errors to share out
 * @param bLambda [in] :: the bin width in lambda
 * @param lambdaMin [in] :: the start of the range to share counts to
 * @param lambdaMax [in] :: the end of the range to share counts to
 * @param outSpecIdx [in] :: the spectrum index to be updated in the output
 * workspace
 * @param IvsLam [in,out] :: the output workspace
 * @param outputE [in,out] :: the projected E values
 */
void shareCounts(
    const double inputCounts, const double inputErr,
    const Mantid::Algorithms::ReflectometrySumInQ::MinMax &lambdaRange,
    Mantid::API::MatrixWorkspace &IvsLam, std::vector<double> &outputE) {
  // Check that we have histogram data
  const auto &outputX = IvsLam.dataX(0);
  auto &outputY = IvsLam.dataY(0);
  if (outputX.size() != outputY.size() + 1) {
    throw std::runtime_error(
        "Expected output array to be histogram data (got X len=" +
        std::to_string(outputX.size()) + ", Y len=" +
        std::to_string(outputY.size()) + ")");
  }

  const double totalWidth = lambdaRange.max - lambdaRange.min;

  // Get the first bin edge in the output X array that is within range.
  // There will probably be some overlap, so start from the bin edge before
  // this (unless we're already at the first bin edge).
  auto startIter = std::lower_bound(outputX.begin(), outputX.end(), lambdaRange.min);
  if (startIter != outputX.begin()) {
    --startIter;
  }

  // Loop through all overlapping output bins. Convert the iterator to an
  // index because we need to index both the X and Y arrays.
  const int xSize = static_cast<int>(outputX.size());
  for (auto outIdx = startIter - outputX.begin(); outIdx < xSize - 1;
       ++outIdx) {
    const double binStart = outputX[outIdx];
    const double binEnd = outputX[outIdx + 1];
    if (binStart > lambdaRange.max) {
      // No longer in the overlap region so we're finished
      break;
    }
    // Add a share of the input counts to this bin based on the proportion of
    // overlap.
    if (totalWidth > Mantid::Kernel::Tolerance) {
      // Share counts out proportionally based on the overlap of this range
      const double overlapWidth =
          std::min({binEnd - binStart, totalWidth, lambdaRange.max - binStart, binEnd - lambdaRange.min});
      const double fraction = overlapWidth / totalWidth;
      outputY[outIdx] += inputCounts * fraction;
      outputE[outIdx] += inputErr * fraction;
    } else {
      // Projection to a single value. Put all counts in the overlapping output
      // bin.
      outputY[outIdx] += inputCounts;
      outputE[outIdx] += inputCounts;
    }
  }
}

double twoThetaWidth(const size_t wsIndex, const Mantid::API::SpectrumInfo &spectrumInfo) {
  const double twoTheta = spectrumInfo.twoTheta(wsIndex);
  if (wsIndex == 0) {
    if (spectrumInfo.size() == 1) {
      throw std::runtime_error("Cannot calculate the two theta range from a single detector only.");
    }
    const double nextTwoTheta = spectrumInfo.twoTheta(1);
    return std::abs(nextTwoTheta - twoTheta);
  } else if (wsIndex == spectrumInfo.size() - 1) {
    const double previousTwoTheta = spectrumInfo.twoTheta(wsIndex - 1);
    return std::abs(twoTheta - previousTwoTheta);
  }
  const double previousTwoTheta = spectrumInfo.twoTheta(wsIndex - 1);
  const double nextTwoTheta = spectrumInfo.twoTheta(wsIndex + 1);
  return std::abs(previousTwoTheta - nextTwoTheta) / 2.;
}
}

namespace Mantid {
namespace Algorithms {

ReflectometrySumInQ::MinMax::MinMax(const double a, const double b) noexcept
  : min(std::min(a, b)), max(std::max(a, b))
{
}

void ReflectometrySumInQ::MinMax::testAndSet(const double a) noexcept {
  if (a < min) {
    min = a;
  }
  if (a > max) {
    max = a;
  }
}

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ReflectometrySumInQ)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string ReflectometrySumInQ::name() const { return "ReflectometrySumInQ"; }

/// Algorithm's version for identification. @see Algorithm::version
int ReflectometrySumInQ::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ReflectometrySumInQ::category() const {
  return "Reflectometry;ILL\\Reflectometry";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ReflectometrySumInQ::summary() const {
  return "Sum counts in lambda along lines of constant Q by projecting to virtual lambda at a reference angle.";
}

/** Initialize the algorithm's properties.
 */
void ReflectometrySumInQ::init() {
  auto inputWSValidator = boost::make_shared<Kernel::CompositeValidator>();
  inputWSValidator->add<API::WorkspaceUnitValidator>("Wavelength");
  inputWSValidator->add<API::InstrumentValidator>();
  auto mandatoryNonnegativeDouble = boost::make_shared<Kernel::CompositeValidator>();
  mandatoryNonnegativeDouble->add<Kernel::MandatoryValidator<double>>();
  auto nonnegativeDouble = boost::make_shared<Kernel::BoundedValidator<double>>();
  nonnegativeDouble->setLower(0.);
  mandatoryNonnegativeDouble->add(nonnegativeDouble);
  auto mandatoryNonnegativeInt = boost::make_shared<Kernel::CompositeValidator>();
  mandatoryNonnegativeInt->add<Kernel::MandatoryValidator<int>>();
  auto nonnegativeInt = boost::make_shared<Kernel::BoundedValidator<int>>();
  nonnegativeInt->setLower(0);
  mandatoryNonnegativeInt->add(nonnegativeInt);
  declareWorkspaceInputProperties<API::MatrixWorkspace, API::IndexType::SpectrumNum | API::IndexType::WorkspaceIndex>(Prop::INPUT_WS, "A workspace in X units of wavelength to be summed.", inputWSValidator);
  declareProperty(
      Kernel::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(Prop::OUTPUT_WS, "",
                                                             Kernel::Direction::Output),
      "A single histogram workspace containing the result of summation in Q.");
  declareProperty(Prop::BEAM_CENTRE, EMPTY_INT(), mandatoryNonnegativeInt, "Fractional workspace index of the specular reflection centre.");
  declareProperty(Prop::WAVELENGTH_MIN, EMPTY_DBL(), mandatoryNonnegativeDouble, "Minimum wavelength in Angstroms.");
  declareProperty(Prop::WAVELENGTH_MAX, EMPTY_DBL(), mandatoryNonnegativeDouble, "Maximum wavelength in Angstroms.");
  declareProperty(Prop::IS_FLAT_SAMPLE, true, "If true, the summation is handled as the standard divergent beam case, otherwise as the non-flat sample case.");
}

/** Execute the algorithm.
 */
void ReflectometrySumInQ::exec() {
  API::MatrixWorkspace_sptr inWS;
  Indexing::SpectrumIndexSet indices;
  std::tie(inWS, indices) = getWorkspaceAndIndices<API::MatrixWorkspace>(Prop::INPUT_WS);
  checkBeamCentreIn(indices);
  auto outWS = sumInQ(*inWS, indices);
  setProperty(Prop::OUTPUT_WS, outWS);
}

std::map<std::string, std::string> ReflectometrySumInQ::validateInputs() {
  std::map<std::string, std::string> issues;
  const double wavelengthMin = getProperty(Prop::WAVELENGTH_MIN);
  const double wavelengthMax = getProperty(Prop::WAVELENGTH_MAX);
  if (wavelengthMin >= wavelengthMax) {
    issues[Prop::WAVELENGTH_MIN] = "Mininum wavelength cannot be greater or equal to maximum wavelength";
  }
  return issues;
}

void ReflectometrySumInQ::checkBeamCentreIn(const Indexing::SpectrumIndexSet &indices) {
  const int beamCentre = getProperty(Prop::BEAM_CENTRE);
  const auto iter = std::find(indices.begin(), indices.end(), static_cast<size_t>(beamCentre));
  if (iter == indices.end()) {
    throw std::runtime_error(Prop::BEAM_CENTRE + " is not included in InputWorkspaceIndexSet.");
  }
}

/**
* Construct an "empty" output workspace in virtual-lambda for summation in Q.
* The workspace will have the same x values as the input workspace but the y
* values will all be zero.
*
* @return : a 1D workspace where y values are all zero
*/
API::MatrixWorkspace_sptr
ReflectometrySumInQ::constructIvsLamWS(const API::MatrixWorkspace &detectorWS, const Indexing::SpectrumIndexSet &indices, const Angles &refAngles) {

  // Calculate the number of bins based on the min/max wavelength, using
  // the same bin width as the input workspace
  const int twoThetaRIdx = getProperty(Prop::BEAM_CENTRE);
  const auto &x = detectorWS.x(static_cast<size_t>(twoThetaRIdx));
  const double binWidth = (x.back() - x.front()) / static_cast<double>(x.size());
  const auto wavelengthRange = findWavelengthMinMax(detectorWS, indices, refAngles);
  if (std::abs(wavelengthRange.max - wavelengthRange.min) < binWidth) {
    throw std::runtime_error("");
  }
  const int numBins = static_cast<int>(
      std::ceil((wavelengthRange.max - wavelengthRange.min) / binWidth));
  // Construct the histogram with these X values. Y and E values are zero.
  const HistogramData::BinEdges bins(numBins, HistogramData::LinearGenerator(wavelengthRange.min, binWidth));
  const HistogramData::Histogram modelHistogram(bins);
  // Create the output workspace
  API::MatrixWorkspace_sptr outputWS = DataObjects::create<DataObjects::Workspace2D>(detectorWS, 1, std::move(modelHistogram));

  // Set the detector ID from the twoThetaR detector.
  auto &outSpec = outputWS->getSpectrum(0);
  // TODO: Handle grouped detectors correctly.
  const detid_t twoThetaRDetID =
      detectorWS.spectrumInfo().detector(static_cast<size_t>(twoThetaRIdx)).getID();
  outSpec.clearDetectorIDs();
  outSpec.addDetectorID(twoThetaRDetID);

  return outputWS;
}

ReflectometrySumInQ::MinMax ReflectometrySumInQ::findWavelengthMinMax(const API::MatrixWorkspace &detectorWS, const Indexing::SpectrumIndexSet &indices, const Angles &refAngles) {
  const double lambdaMin = getProperty(Prop::WAVELENGTH_MIN);
  const double lambdaMax = getProperty(Prop::WAVELENGTH_MAX);
  const API::SpectrumInfo &spectrumInfo = detectorWS.spectrumInfo();
  // Get the new max and min X values of the projected (virtual) lambda range

  // Find minimum and maximum 2thetas and the corresponding indices.
  // It cannot be assumed that 2theta increases with indices, check for example
  // D17 at ILL
  std::pair<size_t, double> twoThetaMin{0, std::numeric_limits<double>::max()};
  std::pair<size_t, double> twoThetaMax{0, std::numeric_limits<double>::lowest()};
  for (const auto i : indices) {
    const auto twoTheta = spectrumInfo.signedTwoTheta(i);
    if (twoTheta < twoThetaMin.second) {
      twoThetaMin.first = i;
      twoThetaMin.second = twoTheta;
    }
    if (twoTheta > twoThetaMax.second) {
      twoThetaMax.first = i;
      twoThetaMax.second = twoTheta;
    }
  }

  MinMax wavelengthRange;
  const double bTwoThetaMin = twoThetaWidth(twoThetaMin.first, spectrumInfo);
  // For bLambda, use the average bin size for this spectrum
  auto xValues = detectorWS.x(twoThetaMin.first);
  double bLambda = (xValues[xValues.size() - 1] - xValues[0]) /
                   static_cast<int>(xValues.size());
  auto r = projectedLambdaRange(lambdaMax, twoThetaMin.second, bLambda, bTwoThetaMin, refAngles);
  wavelengthRange.max = r.max;
  const double bTwoThetaMax = twoThetaWidth(twoThetaMax.first, spectrumInfo);
  xValues = detectorWS.x(twoThetaMax.first);
  bLambda = (xValues[xValues.size() - 1] - xValues[0]) /
            static_cast<int>(xValues.size());
  r = projectedLambdaRange(lambdaMin, twoThetaMax.second, bLambda, bTwoThetaMax, refAngles);
  wavelengthRange.min = r.min;
  if (wavelengthRange.min > wavelengthRange.max) {
    throw std::runtime_error(
        "Error projecting lambda range to reference line; projected range (" +
        std::to_string(wavelengthRange.min) + "," + std::to_string(wavelengthRange.max) +
        ") is negative.");
  }
  return wavelengthRange;
}

/**
* Share counts from an input value onto the projected output in virtual-lambda
*
* @param inputIdx [in] :: the index into the input arrays
* @param twoTheta [in] :: the value of twotTheta for this spectrum
* @param bTwoTheta [in] :: the size of the pixel in twoTheta
* @param inputX [in] :: the input spectrum X values
* @param inputY [in] :: the input spectrum Y values
* @param inputE [in] :: the input spectrum E values
* @param detectors [in] :: spectrum indices of the detectors of interest
* @param outSpecIdx [in] :: the output spectrum index
* @param IvsLam [in,out] :: the output workspace
* @param outputE [in,out] :: the projected E values
*/
void ReflectometrySumInQ::processValue(const int inputIdx, const double twoTheta, const double bTwoTheta,
    const Angles &refAngles, const HistogramData::HistogramX &inputX, const HistogramData::HistogramY &inputY,
    const HistogramData::HistogramE &inputE, API::MatrixWorkspace &IvsLam,
    std::vector<double> &outputE) {

  // Check whether there are any counts (if not, nothing to share)
  const double inputCounts = inputY[inputIdx];
  if (inputCounts <= 0.0 || std::isnan(inputCounts) ||
      std::isinf(inputCounts)) {
    return;
  }
  // Get the bin width and the bin centre
  const double bLambda = inputX[inputIdx + 1] - inputX[inputIdx];
  const double lambda = inputX[inputIdx] + bLambda / 2.0;
  // Project these coordinates onto the virtual-lambda output (at twoThetaR)
  const auto lambdaRange = projectedLambdaRange(lambda, twoTheta, bLambda, bTwoTheta, refAngles);
  // Share the input counts into the output array
  shareCounts(inputCounts, inputE[inputIdx], lambdaRange, IvsLam, outputE);
}

/**
* Project an input pixel onto an arbitrary reference line at twoThetaR. The
* projection is done along lines of constant Q, which emanate from theta0. The
* top-left and bottom-right corners of the pixel are projected, resulting in an
* output range in "virtual" lambda (lambdaV).
*
* For a description of this projection, see:
*   R. Cubitt, T. Saerbeck, R.A. Campbell, R. Barker, P. Gutfreund
*   J. Appl. Crystallogr., 48 (6) (2015)
*
* @param lambda [in] :: the lambda coord of the centre of the pixel to project
* @param twoTheta [in] :: the twoTheta coord of the centre of the pixel to
*project
* @param bLambda [in] :: the pixel size in lambda
* @param bTwoTheta [in] :: the pixel size in twoTheta
* @param detectors [in] :: spectrum indices of the detectors of interest
* @param lambdaVMin [out] :: the projected range start
* @param lambdaVMax [out] :: the projected range end
*/
ReflectometrySumInQ::MinMax ReflectometrySumInQ::projectedLambdaRange(const double lambda, const double twoTheta, const double bLambda,
    const double bTwoTheta, const Angles &refAngles) {

  // We cannot project pixels below the horizon angle
  if (twoTheta <= refAngles.horizon) {
    throw std::runtime_error("Cannot process twoTheta=" +
                             std::to_string(twoTheta * 180.0 / M_PI) +
                             " as it is below the horizon angle=" +
                             std::to_string(refAngles.horizon * 180.0 / M_PI));
  }

  // Get the distance from the pixel to twoThetaR
  const double gamma = twoTheta - refAngles.twoTheta;

  // Calculate the projected wavelength range
  MinMax range;
  try {
    const double lambdaTop =
        (lambda + bLambda / 2.0) *
         std::sin(refAngles.delta) /
         std::sin(refAngles.delta + gamma - bTwoTheta / 2.0);
    const double lambdaBot =
        (lambda - bLambda / 2.0) *
         std::sin(refAngles.delta) /
         std::sin(refAngles.delta + gamma + bTwoTheta / 2.0);
    range.testAndSet(lambdaBot);
    range.testAndSet(lambdaTop);
  } catch (std::exception &ex) {
    throw std::runtime_error(
        "Failed to project (lambda, twoTheta) = (" + std::to_string(lambda) +
        "," + std::to_string(twoTheta * 180.0 / M_PI) + ") onto twoThetaR = " +
        std::to_string(refAngles.twoTheta) + ": " + ex.what());
  }
  return range;
}

ReflectometrySumInQ::Angles ReflectometrySumInQ::referenceAngles(const API::SpectrumInfo &spectrumInfo) {
  Angles a;
  const int beamCentre = getProperty(Prop::BEAM_CENTRE);
  const double centreTwoTheta = spectrumInfo.signedTwoTheta(static_cast<size_t>(beamCentre));
  const bool isFlat = getProperty(Prop::IS_FLAT_SAMPLE);
  if (isFlat) {
    a.horizon = centreTwoTheta / 2.;
  } else {
    a.horizon = 0.;
  }
  a.twoTheta = centreTwoTheta;
  a.delta = a.twoTheta - a.horizon;
  return a;
}

/**
* Sum counts from the input workspace in lambda along lines of constant Q by
* projecting to "virtual lambda" at a reference angle twoThetaR.
*
* @param detectorWS [in] :: the input workspace in wavelength
* @return :: the output workspace in wavelength
*/
API::MatrixWorkspace_sptr
ReflectometrySumInQ::sumInQ(const API::MatrixWorkspace &detectorWS, const Indexing::SpectrumIndexSet &indices) {

  const auto spectrumInfo = detectorWS.spectrumInfo();
  const auto refAngles = referenceAngles(spectrumInfo);
  // Construct the output array in virtual lambda
  API::MatrixWorkspace_sptr IvsLam = constructIvsLamWS(detectorWS, indices, refAngles);
  auto &outputE = IvsLam->dataE(0);
  // Loop through each spectrum in the detector group
  for (auto spIdx : indices) {
    // Get the angle of this detector and its size in twoTheta
    const double twoTheta = spectrumInfo.signedTwoTheta(spIdx);
    const double bTwoTheta = twoThetaWidth(spIdx, spectrumInfo);
    // Check X length is Y length + 1
    const auto &inputX = detectorWS.x(spIdx);
    const auto &inputY = detectorWS.y(spIdx);
    const auto &inputE = detectorWS.e(spIdx);
    if (inputX.size() != inputY.size() + 1) {
      throw std::runtime_error(
          "Expected input workspace to be histogram data (got X len=" +
          std::to_string(inputX.size()) + ", Y len=" +
          std::to_string(inputY.size()) + ")");
    }
    // Create a vector for the projected errors for this spectrum.
    // (Output Y values can simply be accumulated directly into the output
    // workspace, but for error values we need to create a separate error
    // vector for the projected errors from each input spectrum and then
    // do an overall sum in quadrature.)
    std::vector<double> projectedE(outputE.size(), 0.0);
    // Process each value in the spectrum
    const int ySize = static_cast<int>(inputY.size());
    for (int inputIdx = 0; inputIdx < ySize; ++inputIdx) {
      // Do the summation in Q
      processValue(inputIdx, twoTheta, bTwoTheta, refAngles, inputX, inputY,
                          inputE, *IvsLam, projectedE);
    }
    // Sum errors in quadrature
    const int eSize = static_cast<int>(outputE.size());
    for (int outIdx = 0; outIdx < eSize; ++outIdx) {
      outputE[outIdx] += projectedE[outIdx] * projectedE[outIdx];
    }
  }

  // Take the square root of all the accumulated squared errors for this
  // detector group. Assumes Gaussian errors
  double (*rs)(double) = std::sqrt;
  std::transform(outputE.begin(), outputE.end(), outputE.begin(), rs);

  return IvsLam;
}

} // namespace Algorithms
} // namespace Mantid
