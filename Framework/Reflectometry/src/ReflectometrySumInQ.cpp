// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidReflectometry/ReflectometrySumInQ.h"

#include "MantidAPI/Algorithm.hxx"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Crystal/AngleUnits.h"
#include "MantidGeometry/IDetector.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidIndexing/SpectrumNumber.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/Strings.h"

namespace {
/// String constants for the algorithm's properties.
namespace Prop {
const static std::string BEAM_CENTRE{"BeamCentre"};
const static std::string INPUT_WS{"InputWorkspace"};
const static std::string IS_FLAT_SAMPLE{"FlatSample"};
const static std::string OUTPUT_WS{"OutputWorkspace"};
const static std::string PARTIAL_BINS{"IncludePartialBins"};
} // namespace Prop

/**
 * Interpolate the 2theta of a given fractional workspace index.
 * @param wsIndex a fractional workspace index
 * @param spectrumInfo a spectrum info
 * @return the interpolated 2theta in radians
 */
double centreTwoTheta(const double wsIndex, const Mantid::API::SpectrumInfo &spectrumInfo) {
  double twoTheta;
  const size_t index{static_cast<size_t>(wsIndex)};
  if (wsIndex == static_cast<double>(index)) {
    twoTheta = spectrumInfo.twoTheta(index);
  } else {
    // Linear interpolation. Straight from Wikipedia.
    const double x0{static_cast<double>(index)};
    const double x1{static_cast<double>(index + 1)};
    const double y0{spectrumInfo.twoTheta(index)};
    const double y1{spectrumInfo.twoTheta(index + 1)};
    twoTheta = (y0 * (x1 - wsIndex) + y1 * (wsIndex - x0)) / (x1 - x0);
  }
  return twoTheta;
}

/**
 * Project a wavelength to given reference angle by keeping the momentum
 * transfer constant.
 * @param wavelength the wavelength to project
 * @param twoTheta a 2theta angle
 * @param refAngles the reference angles for the projection
 * @return a projected wavelength
 */
double projectToReference(const double wavelength, const double twoTheta,
                          const Mantid::Reflectometry::ReflectometrySumInQ::Angles &refAngles) {
  return wavelength * std::sin(refAngles.delta) / std::sin(twoTheta - refAngles.horizon);
}

/**
 * Share the given input counts into the output array bins proportionally
 * according to how much the bins overlap the given lambda range.
 * outputX.size() must equal to outputY.size() + 1
 *
 * @param inputCounts [in] :: the input counts to share out
 * @param inputErr [in] :: the input errors to share out
 * @param lambdaRange [in] :: the width of the input in virtual lambda
 * @param IvsLam [in,out] :: the output workspace
 * @param outputE [in,out] :: the projected E values
 */
void shareCounts(const double inputCounts, const double inputErr,
                 const Mantid::Reflectometry::ReflectometrySumInQ::MinMax &lambdaRange,
                 Mantid::API::MatrixWorkspace &IvsLam, std::vector<double> &outputE) {
  // Check that we have histogram data
  const auto &outputX = IvsLam.x(0);
  auto &outputY = IvsLam.mutableY(0);

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
  const auto xSize = static_cast<int>(outputX.size());
  for (auto outIdx = startIter - outputX.begin(); outIdx < xSize - 1; ++outIdx) {
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

/**
 * Return the angular 2theta width of a pixel.
 *
 * @param wsIndex [in] :: a workspace index to spectrumInfo
 * @param spectrumInfo [in] :: a spectrum info structure
 * @return :: the pixel's angular width in radians
 */
Mantid::Reflectometry::ReflectometrySumInQ::MinMax twoThetaWidth(const size_t wsIndex,
                                                                 const Mantid::API::SpectrumInfo &spectrumInfo) {
  const double twoTheta = spectrumInfo.twoTheta(wsIndex);
  Mantid::Reflectometry::ReflectometrySumInQ::MinMax range;
  if (wsIndex == 0) {
    if (spectrumInfo.size() <= 1) {
      throw std::runtime_error("Cannot calculate pixel widths from a workspace "
                               "containing a single histogram.");
    }
    const auto nextTwoTheta = spectrumInfo.twoTheta(1);
    const auto d = std::abs(nextTwoTheta - twoTheta) / 2.;
    range.min = twoTheta - d;
    range.max = twoTheta + d;
  } else if (wsIndex == spectrumInfo.size() - 1) {
    const auto previousTwoTheta = spectrumInfo.twoTheta(wsIndex - 1);
    const auto d = std::abs(twoTheta - previousTwoTheta) / 2.;
    range.min = twoTheta - d;
    range.max = twoTheta + d;
  } else {
    const auto t1 = spectrumInfo.twoTheta(wsIndex - 1);
    const auto t2 = spectrumInfo.twoTheta(wsIndex + 1);
    const Mantid::Reflectometry::ReflectometrySumInQ::MinMax neighbours(t1, t2);
    range.min = (twoTheta + neighbours.min) / 2.;
    range.max = (twoTheta + neighbours.max) / 2.;
  }
  return range;
}
} // namespace

namespace Mantid::Reflectometry {

using namespace API;
/**
 * Construct a new MinMax object.
 * The minimum of the arguments is assigned to the `min` field and
 * maximum to the `max` field.
 *
 * @param a [in] :: a number
 * @param b [in] :: a number
 **/
ReflectometrySumInQ::MinMax::MinMax(const double a, const double b) noexcept
    : min(std::min(a, b)), max(std::max(a, b)) {}

/**
 * Set the `min` and `max` fields if `a` is smaller than `min` and/or
 * geater than `max`.
 *
 * @param a [in] :: a number
 */
void ReflectometrySumInQ::MinMax::testAndSet(const double a) noexcept {
  if (a < min) {
    min = a;
  }
  if (a > max) {
    max = a;
  }
}

/**
 * Set the `max` field if `a` is geater than `max`.
 *
 * @param a [in] :: a number
 */
void ReflectometrySumInQ::MinMax::testAndSetMax(const double a) noexcept { max = std::max(max, a); }

/**
 * Set the `min` field if `a` is smaller than `min`.
 *
 * @param a [in] :: a number
 */
void ReflectometrySumInQ::MinMax::testAndSetMin(const double a) noexcept { min = std::min(min, a); }

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ReflectometrySumInQ)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string ReflectometrySumInQ::name() const { return "ReflectometrySumInQ"; }

/// Algorithm's version for identification. @see Algorithm::version
int ReflectometrySumInQ::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ReflectometrySumInQ::category() const { return "Reflectometry;ILL\\Reflectometry"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ReflectometrySumInQ::summary() const {
  return "Sum counts in lambda along lines of constant Q by projecting to "
         "virtual lambda at a reference angle.";
}

/** Initialize the algorithm's properties.
 */
void ReflectometrySumInQ::init() {
  auto inputWSValidator = std::make_shared<Kernel::CompositeValidator>();
  inputWSValidator->add<API::WorkspaceUnitValidator>("Wavelength");
  inputWSValidator->add<API::InstrumentValidator>();
  auto mandatoryNonnegative = std::make_shared<Kernel::CompositeValidator>();
  mandatoryNonnegative->add<Kernel::MandatoryValidator<double>>();
  auto nonnegative = std::make_shared<Kernel::BoundedValidator<double>>();
  nonnegative->setLower(0.);
  mandatoryNonnegative->add(nonnegative);
  declareWorkspaceInputProperties<API::MatrixWorkspace, static_cast<int>(IndexType::SpectrumNum) |
                                                            static_cast<int>(IndexType::WorkspaceIndex)>(
      Prop::INPUT_WS, "A workspace in X units of wavelength to be summed.", inputWSValidator);
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(Prop::OUTPUT_WS, "", Kernel::Direction::Output),
      "A single histogram workspace containing the result of summation in Q.");
  declareProperty(Prop::BEAM_CENTRE, EMPTY_DBL(), mandatoryNonnegative,
                  "Fractional workspace index of the specular reflection centre.");
  declareProperty(Prop::IS_FLAT_SAMPLE, true,
                  "If true, the summation is handled as the standard divergent "
                  "beam case, otherwise as the non-flat sample case.");
  declareProperty(Prop::PARTIAL_BINS, false,
                  "If true, use the full projected wavelength range possibly "
                  "including partially filled bins.");
}

/** Execute the algorithm.
 */
void ReflectometrySumInQ::exec() {
  API::MatrixWorkspace_sptr inWS;
  Indexing::SpectrumIndexSet indices;
  std::tie(inWS, indices) = getWorkspaceAndIndices<API::MatrixWorkspace>(Prop::INPUT_WS);
  auto outWS = sumInQ(*inWS, indices);
  if (inWS->isDistribution()) {
    API::WorkspaceHelpers::makeDistribution(outWS);
  }
  setProperty(Prop::OUTPUT_WS, outWS);
}

/// Validate the some of the algorithm's input properties.
std::map<std::string, std::string> ReflectometrySumInQ::validateInputs() {
  std::map<std::string, std::string> issues;
  API::MatrixWorkspace_sptr inWS;
  Indexing::SpectrumIndexSet indices;

  // validateInputs is called on the individual workspaces when the algorithm
  // is executed, it but may get called on a group from AlgorithmDialog. This
  // isn't handled in getWorkspaceAndIndices. We should fix this properly but
  // for now skip validation for groups to avoid an exception. See #22933
  try {
    std::tie(inWS, indices) = getWorkspaceAndIndices<API::MatrixWorkspace>(Prop::INPUT_WS);
  } catch (std::runtime_error &) {
    return issues;
  }

  const auto &spectrumInfo = inWS->spectrumInfo();
  const double beamCentre = getProperty(Prop::BEAM_CENTRE);
  const auto beamCentreIndex = static_cast<size_t>(std::round(beamCentre));
  bool beamCentreFound{false};
  for (const auto i : indices) {
    if (spectrumInfo.isMonitor(i)) {
      issues["InputWorkspaceIndexSet"] = "Index set cannot include monitors.";
      break;
    } else if ((i > 0 && spectrumInfo.isMonitor(i - 1)) ||
               (i < spectrumInfo.size() - 1 && spectrumInfo.isMonitor(i + 1))) {
      issues["InputWorkspaceIndexSet"] = "A neighbour to any detector in the index set cannot be a monitor";
      break;
    }
    if (i == beamCentreIndex) {
      beamCentreFound = true;
      break;
    }
  }
  if (!beamCentreFound) {
    issues[Prop::BEAM_CENTRE] = "Beam centre is not included in InputWorkspaceIndexSet.";
  }
  return issues;
}

/**
 * Construct an "empty" output workspace in virtual-lambda for summation in Q.
 *
 * @param detectorWS [in] :: the input workspace
 * @param indices [in] :: the workspace indices of the foreground histograms
 * @param refAngles [in] :: the reference angles
 * @return :: a 1D workspace where y values are all zero
 */
API::MatrixWorkspace_sptr ReflectometrySumInQ::constructIvsLamWS(const API::MatrixWorkspace &detectorWS,
                                                                 const Indexing::SpectrumIndexSet &indices,
                                                                 const Angles &refAngles) {

  // Calculate the number of bins based on the min/max wavelength, using
  // the same bin width as the input workspace
  const auto &edges = detectorWS.binEdges(refAngles.referenceWSIndex);
  const double binWidth = (edges.back() - edges.front()) / static_cast<double>(edges.size() - 1);
  const auto wavelengthRange = findWavelengthMinMax(detectorWS, indices, refAngles);
  if (std::abs(wavelengthRange.max - wavelengthRange.min) < binWidth) {
    throw std::runtime_error("Given wavelength range too small.");
  }
  const auto numBins = static_cast<int>(std::ceil((wavelengthRange.max - wavelengthRange.min) / binWidth));
  // Construct the histogram with these X values. Y and E values are zero.
  const HistogramData::BinEdges bins(numBins + 1, HistogramData::LinearGenerator(wavelengthRange.min, binWidth));
  const HistogramData::Counts counts(numBins, 0.);
  const HistogramData::Histogram modelHistogram(bins, counts);
  // Create the output workspace
  API::MatrixWorkspace_sptr outputWS = DataObjects::create<DataObjects::Workspace2D>(detectorWS, 1, modelHistogram);

  // Set the detector IDs and specturm number from the twoThetaR detector.
  const auto &thetaSpec = detectorWS.getSpectrum(refAngles.referenceWSIndex);
  auto &outSpec = outputWS->getSpectrum(0);
  outSpec.clearDetectorIDs();
  outSpec.addDetectorIDs(thetaSpec.getDetectorIDs());
  outSpec.setSpectrumNo(thetaSpec.getSpectrumNo());

  return outputWS;
}

/**
 * Return the wavelength range of the output histogram.
 * @param detectorWS [in] :: the input workspace
 * @param indices [in] :: the workspace indices of foreground histograms
 * @param refAngles [in] :: the reference angles
 * @return :: the minimum and maximum virtual wavelengths
 */
ReflectometrySumInQ::MinMax ReflectometrySumInQ::findWavelengthMinMax(const API::MatrixWorkspace &detectorWS,
                                                                      const Indexing::SpectrumIndexSet &indices,
                                                                      const Angles &refAngles) {
  const API::SpectrumInfo &spectrumInfo = detectorWS.spectrumInfo();
  // Get the new max and min X values of the projected (virtual) lambda range
  const bool includePartialBins = getProperty(Prop::PARTIAL_BINS);
  // Find minimum and maximum 2thetas and the corresponding indices.
  // It cannot be assumed that 2theta increases with indices, check for example
  // D17 at ILL
  MinMax inputLambdaRange;
  MinMax inputTwoThetaRange;
  for (const auto i : indices) {
    const auto twoThetas = twoThetaWidth(i, spectrumInfo);
    inputTwoThetaRange.testAndSetMin(includePartialBins ? twoThetas.min : twoThetas.max);
    inputTwoThetaRange.testAndSetMax(includePartialBins ? twoThetas.max : twoThetas.min);
    const auto &edges = detectorWS.binEdges(i);
    for (size_t xIndex = 0; xIndex < edges.size(); ++xIndex) {
      // It is common for the wavelength to have negative values at ILL.
      const auto x = edges[xIndex + (includePartialBins ? 0 : 1)];
      if (x > 0.) {
        inputLambdaRange.testAndSet(x);
        break;
      }
    }
    if (includePartialBins) {
      inputLambdaRange.testAndSet(edges.back());
    } else {
      inputLambdaRange.testAndSet(edges[edges.size() - 2]);
    }
  }

  MinMax outputLambdaRange;
  outputLambdaRange.min = projectToReference(inputLambdaRange.min, inputTwoThetaRange.max, refAngles);
  outputLambdaRange.max = projectToReference(inputLambdaRange.max, inputTwoThetaRange.min, refAngles);
  if (outputLambdaRange.min > outputLambdaRange.max) {
    throw std::runtime_error("Error projecting lambda range to reference line; projected range (" +
                             std::to_string(outputLambdaRange.min) + "," + std::to_string(outputLambdaRange.max) +
                             ") is negative.");
  }
  return outputLambdaRange;
}

/**
 * Share counts from an input value onto the projected output in virtual-lambda
 *
 * @param inputIdx [in] :: the index of the input histogram
 * @param twoThetaRange [in] :: the 2theta width of the pixel
 * @param refAngles [in] :: the reference 2theta angles
 * @param edges [in] :: the input spectrum bin edges
 * @param counts [in] :: the input spectrum counts
 * @param stdDevs [in] :: the input spectrum count standard deviations
 * @param IvsLam [in,out] :: the output workspace
 * @param outputE [in,out] :: the projected E values
 */
void ReflectometrySumInQ::processValue(const int inputIdx, const MinMax &twoThetaRange, const Angles &refAngles,
                                       const HistogramData::BinEdges &edges, const HistogramData::Counts &counts,
                                       const HistogramData::CountStandardDeviations &stdDevs,
                                       API::MatrixWorkspace &IvsLam, std::vector<double> &outputE) {

  // Check whether there are any counts (if not, nothing to share)
  const double inputCounts = counts[inputIdx];
  if (edges[inputIdx] < 0. || inputCounts <= 0.0 || std::isnan(inputCounts) || std::isinf(inputCounts)) {
    return;
  }
  // Get the bin width and the bin centre
  const MinMax wavelengthRange(edges[inputIdx], edges[inputIdx + 1]);
  // Project these coordinates onto the virtual-lambda output (at twoThetaR)
  const auto lambdaRange = projectedLambdaRange(wavelengthRange, twoThetaRange, refAngles);
  // Share the input counts into the output array
  shareCounts(inputCounts, stdDevs[inputIdx], lambdaRange, IvsLam, outputE);
}

/**
 * Project an input pixel onto an arbitrary reference line at a reference angle.
 * The projection is done along lines of constant Q, which emanate from the
 * horizon angle at wavelength = 0. The top-left and bottom-right corners of
 * the pixel are projected, resulting in an output range in "virtual" lambda.
 *
 * For a description of this projection, see:
 *   R. Cubitt, T. Saerbeck, R.A. Campbell, R. Barker, P. Gutfreund
 *   J. Appl. Crystallogr., 48 (6) (2015)
 *
 * @param wavelengthRange [in] :: the bin edges of the input bin
 * @param twoThetaRange [in] :: the 2theta width of the pixel
 * @param refAngles [in] :: the reference angles
 * @return :: the projected wavelength range
 */
ReflectometrySumInQ::MinMax ReflectometrySumInQ::projectedLambdaRange(const MinMax &wavelengthRange,
                                                                      const MinMax &twoThetaRange,
                                                                      const Angles &refAngles) {

  // We cannot project pixels below the horizon angle
  if (twoThetaRange.min <= refAngles.horizon) {
    const auto twoTheta = (twoThetaRange.min + twoThetaRange.max) / 2.;
    throw std::runtime_error(
        "Cannot process twoTheta=" + std::to_string(twoTheta * Geometry::rad2deg) +
        " as it is below the horizon angle=" + std::to_string(refAngles.horizon * Geometry::rad2deg));
  }

  // Calculate the projected wavelength range
  MinMax range;
  range.max = projectToReference(wavelengthRange.max, twoThetaRange.min, refAngles);
  range.min = projectToReference(wavelengthRange.min, twoThetaRange.max, refAngles);
  return range;
}

/**
 * Return the reference 2theta angle and the corresponding horizon angle.
 *
 * @param spectrumInfo [in] :: a spectrum info of the input workspace.
 * @return :: the reference angle struct
 */
ReflectometrySumInQ::Angles ReflectometrySumInQ::referenceAngles(const API::SpectrumInfo &spectrumInfo) {
  Angles a;
  const double beamCentre = getProperty(Prop::BEAM_CENTRE);
  const bool isFlat = getProperty(Prop::IS_FLAT_SAMPLE);
  const double twoTheta = centreTwoTheta(beamCentre, spectrumInfo);
  a.referenceWSIndex = static_cast<size_t>(beamCentre);
  a.twoTheta = twoTheta;
  if (isFlat) {
    a.horizon = twoTheta / 2.;
  } else {
    a.horizon = 0.;
  }
  a.delta = a.twoTheta - a.horizon;
  return a;
}

/**
 * Sum counts from the input workspace in lambda along lines of constant Q by
 * projecting to "virtual lambda" at a reference angle.
 *
 * @param detectorWS [in] :: the input workspace in wavelength
 * @param indices [in] :: an index set defining the foreground histograms
 * @return :: the single histogram output workspace in wavelength
 */
API::MatrixWorkspace_sptr ReflectometrySumInQ::sumInQ(const API::MatrixWorkspace &detectorWS,
                                                      const Indexing::SpectrumIndexSet &indices) {

  const auto spectrumInfo = detectorWS.spectrumInfo();
  const auto refAngles = referenceAngles(spectrumInfo);
  // Construct the output workspace in virtual lambda
  API::MatrixWorkspace_sptr IvsLam = constructIvsLamWS(detectorWS, indices, refAngles);
  auto &outputE = IvsLam->mutableE(0);
  // Loop through each spectrum in the detector group
  for (const auto spIdx : indices) {
    if (spectrumInfo.isMasked(spIdx) || spectrumInfo.isMonitor(spIdx)) {
      continue;
    }
    // Get the size of this detector in twoTheta
    const auto twoThetaRange = twoThetaWidth(spIdx, spectrumInfo);
    const auto inputBinEdges = detectorWS.binEdges(spIdx);
    const auto inputCounts = detectorWS.counts(spIdx);
    const auto inputStdDevs = detectorWS.countStandardDeviations(spIdx);
    // Create a vector for the projected errors for this spectrum.
    // (Output Y values can simply be accumulated directly into the output
    // workspace, but for error values we need to create a separate error
    // vector for the projected errors from each input spectrum and then
    // do an overall sum in quadrature.)
    std::vector<double> projectedE(outputE.size(), 0.0);
    // Process each value in the spectrum
    const auto ySize = static_cast<int>(inputCounts.size());
    for (int inputIdx = 0; inputIdx < ySize; ++inputIdx) {
      // Do the summation in Q
      processValue(inputIdx, twoThetaRange, refAngles, inputBinEdges, inputCounts, inputStdDevs, *IvsLam, projectedE);
    }
    // Sum errors in quadrature
    const auto eSize = static_cast<int>(outputE.size());
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

} // namespace Mantid::Reflectometry
