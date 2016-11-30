#include "MantidAlgorithms/Qxy.h"
#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidAlgorithms/GravitySANSHelper.h"
#include "MantidAlgorithms/Qhelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/VectorHelper.h"

#include <algorithm>
#include <cmath>

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(Qxy)

using namespace Kernel;
using namespace API;
using namespace Geometry;

void Qxy::init() {
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<HistogramValidator>();
  wsValidator->add<InstrumentValidator>();

  declareProperty(make_unique<WorkspaceProperty<>>(
                      "InputWorkspace", "", Direction::Input, wsValidator),
                  "The corrected data in units of wavelength.");
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "The name to use for the corrected workspace.");

  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(1.0e-12);

  declareProperty(
      "MaxQxy", -1.0, mustBePositive,
      "The upper limit of the Qx-Qy grid (goes from -MaxQxy to +MaxQxy).");
  declareProperty("DeltaQ", -1.0, mustBePositive,
                  "The dimension of a Qx-Qy cell.");
  declareProperty(make_unique<WorkspaceProperty<>>(
                      "PixelAdj", "", Direction::Input, PropertyMode::Optional),
                  "The scaling to apply to each spectrum e.g. for detector "
                  "efficiency, must have just one bin per spectrum and the "
                  "same number of spectra as DetBankWorkspace.");
  auto wavVal = boost::make_shared<CompositeValidator>();
  wavVal->add<WorkspaceUnitValidator>("Wavelength");
  wavVal->add<HistogramValidator>();
  declareProperty(
      make_unique<WorkspaceProperty<>>("WavelengthAdj", "", Direction::Input,
                                       PropertyMode::Optional, wavVal),
      "The scaling to apply to each bin to account for monitor "
      "counts, transmission fraction, etc. Must be one spectrum "
      "with the same binning as the InputWorkspace, the same units "
      "(counts) and the same [[ConvertToDistribution|distribution "
      "status]].");
  declareProperty("AccountForGravity", false,
                  "Whether to correct for the effects of gravity.",
                  Direction::Input);
  declareProperty("SolidAngleWeighting", true,
                  "If true, pixels will be weighted by their solid angle.",
                  Direction::Input);
  auto mustBePositive2 = boost::make_shared<BoundedValidator<double>>();
  mustBePositive2->setLower(0.0);
  declareProperty("RadiusCut", 0.0, mustBePositive2,
                  "The minimum distance in mm from the beam center at which "
                  "all wavelengths are used in the calculation (see section "
                  "[[Q1D#Resolution and Cutoffs|Resolution and Cutoffs]])");
  declareProperty("WaveCut", 0.0, mustBePositive2,
                  "The shortest wavelength in angstrom at which counts should "
                  "be summed from all detector pixels (see section "
                  "[[Q1D#Resolution and Cutoffs|Resolution and Cutoffs]])");
  declareProperty("OutputParts", false,
                  "Set to true to output two additional workspaces which will "
                  "have the names OutputWorkspace_sumOfCounts "
                  "OutputWorkspace_sumOfNormFactors. The division of "
                  "_sumOfCounts and _sumOfNormFactors equals the workspace "
                  "returned by the property OutputWorkspace");
  declareProperty("ExtraLength", 0.0, mustBePositive2,
                  "Additional length for gravity correction.");
}

void Qxy::exec() {
  MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");
  MatrixWorkspace_const_sptr waveAdj = getProperty("WavelengthAdj");
  MatrixWorkspace_const_sptr pixelAdj = getProperty("PixelAdj");
  const bool doGravity = getProperty("AccountForGravity");
  const bool doSolidAngle = getProperty("SolidAngleWeighting");

  // throws if we don't have common binning or another incompatibility
  Qhelper helper;
  helper.examineInput(inputWorkspace, waveAdj, pixelAdj);
  g_log.debug() << "All input workspaces were found to be valid\n";

  // Create the output Qx-Qy grid
  MatrixWorkspace_sptr outputWorkspace =
      this->setUpOutputWorkspace(inputWorkspace);

  // Will also need an identically-sized workspace to hold the solid angle/time
  // bin masked weight
  MatrixWorkspace_sptr weights =
      WorkspaceFactory::Instance().create(outputWorkspace);
  // Copy the X values from the output workspace to the solidAngles one
  for (size_t i = 0; i < weights->getNumberHistograms(); ++i)
    weights->setSharedX(i, outputWorkspace->sharedX(0));

  const size_t numSpec = inputWorkspace->getNumberHistograms();
  const size_t numBins = inputWorkspace->blocksize();

  // Set the progress bar (1 update for every one percent increase in progress)
  Progress prog(this, 0.05, 1.0, numSpec);

  const auto &spectrumInfo = inputWorkspace->spectrumInfo();

  // the samplePos is often not (0, 0, 0) because the instruments components are
  // moved to account for the beam centre
  const V3D samplePos = spectrumInfo.samplePosition();

  for (int64_t i = 0; i < int64_t(numSpec); ++i) {
    if (!spectrumInfo.hasDetectors(i)) {
      g_log.warning() << "Workspace index " << i
                      << " has no detector assigned to it - discarding\n";
      continue;
    }
    // If no detector found or if it's masked or a monitor, skip onto the next
    // spectrum
    if (spectrumInfo.isMonitor(i) || spectrumInfo.isMasked(i))
      continue;

    // get the bins that are included inside the RadiusCut/WaveCutcut off, those
    // to calculate for
    const size_t wavStart = helper.waveLengthCutOff(
        inputWorkspace, getProperty("RadiusCut"), getProperty("WaveCut"), i);
    if (wavStart >= inputWorkspace->y(i).size()) {
      // all the spectra in this detector are out of range
      continue;
    }

    V3D detPos = spectrumInfo.position(i) - samplePos;

    // these will be re-calculated if gravity is on but without gravity there is
    // no need
    double phi = atan2(detPos.Y(), detPos.X());
    double a = cos(phi);
    double b = sin(phi);
    double sinTheta = sin(spectrumInfo.twoTheta(i) * 0.5);

    // Get references to the data for this spectrum
    const auto &X = inputWorkspace->x(i);
    const auto &Y = inputWorkspace->y(i);
    const auto &E = inputWorkspace->e(i);

    const auto &axis = outputWorkspace->x(0);

    // the solid angle of the detector as seen by the sample is used for
    // normalisation later on
    double angle = spectrumInfo.detector(i).solidAngle(samplePos);

    // some bins are masked completely or partially, the following vector will
    // contain the fractions
    std::vector<double> maskFractions;
    if (inputWorkspace->hasMaskedBins(i)) {
      // go through the set and convert it to a vector
      const MatrixWorkspace::MaskList &mask = inputWorkspace->maskedBins(i);
      maskFractions.resize(numBins, 1.0);
      MatrixWorkspace::MaskList::const_iterator it, itEnd(mask.end());
      for (it = mask.begin(); it != itEnd; ++it) {
        // The weight for this masked bin is 1 minus the degree to which this
        // bin is masked
        maskFractions[it->first] -= it->second;
      }
    }
    double maskFraction(1);

    // this object is not used if gravity correction is off, but it is only
    // constructed once per spectrum
    GravitySANSHelper grav;
    if (doGravity) {
      grav = GravitySANSHelper(spectrumInfo, i, getProperty("ExtraLength"));
    }

    for (int j = static_cast<int>(numBins) - 1; j >= static_cast<int>(wavStart);
         --j) {
      if (j < 0)
        break; // Be careful with counting down. Need a better fix but this will
               // work for now
      const double binWidth = X[j + 1] - X[j];
      // Calculate the wavelength at the mid-point of this bin
      const double wavLength = X[j] + (binWidth) / 2.0;

      if (doGravity) {
        // SANS instruments must have their y-axis pointing up, show the
        // detector position as where the neutron would be without gravity
        sinTheta = grav.calcComponents(wavLength, a, b);
      }

      // Calculate |Q| for this bin
      const double Q = 4.0 * M_PI * sinTheta / wavLength;

      // Now get the x & y components of Q.
      const double Qx = a * Q;
      // Test whether they're in range, if not go to next spectrum.
      if (Qx < axis.front() || Qx >= axis.back())
        break;
      const double Qy = b * Q;
      if (Qy < axis.front() || Qy >= axis.back())
        break;
      // Find the indices pointing to the place in the 2D array where this bin's
      // contents should go
      const auto xIndex =
          std::upper_bound(axis.begin(), axis.end(), Qx) - axis.begin() - 1;
      const int yIndex = static_cast<int>(
          std::upper_bound(axis.begin(), axis.end(), Qy) - axis.begin() - 1);
      //      PARALLEL_CRITICAL(qxy)    /* Write to shared memory - must protect
      //      */
      {
        // the data will be copied to this bin in the output array
        double &outputBinY = outputWorkspace->mutableY(yIndex)[xIndex];
        double &outputBinE = outputWorkspace->mutableE(yIndex)[xIndex];

        if (std::isnan(outputBinY)) {
          outputBinY = outputBinE = 0;
        }
        // Add the contents of the current bin to the 2D array.
        outputBinY += Y[j];
        // add the errors in quadranture
        outputBinE = std::sqrt((outputBinE * outputBinE) + (E[j] * E[j]));

        // account for masked bins
        if (!maskFractions.empty()) {
          maskFraction = maskFractions[j];
        }
        // add the total weight for this bin in the weights workspace,
        // in an equivalent bin to where the data was stored

        // first take into account the product of contributions to the weight
        // which have
        // no errors
        double weight = 0.0;
        if (doSolidAngle)
          weight = maskFraction * angle;
        else
          weight = maskFraction;

        // then the product of contributions which have errors, i.e. optional
        // pixelAdj and waveAdj contributions
        auto &outWeightsY = weights->mutableY(yIndex);
        auto &outWeightsE = weights->mutableE(yIndex);

        if (pixelAdj && waveAdj) {
          auto pixelY = pixelAdj->y(i)[0];
          auto pixelE = pixelAdj->e(i)[0];

          auto waveY = waveAdj->y(0)[j];
          auto waveE = waveAdj->e(0)[j];

          outWeightsY[xIndex] += weight * pixelY * waveY;
          const double pixelYSq = pixelY * pixelY;
          const double pixelESq = pixelE * pixelE;
          const double waveYSq = waveY * waveY;
          const double waveESq = waveE * waveE;
          // add product of errors from pixelAdj and waveAdj (note no error on
          // weight is assumed)
          outWeightsE[xIndex] +=
              weight * weight * (waveESq * pixelYSq + pixelESq * waveYSq);
        } else if (pixelAdj) {
          auto pixelY = pixelAdj->y(i)[0];
          auto pixelE = pixelAdj->e(i)[0];

          outWeightsY[xIndex] += weight * pixelY;
          const double pixelESq = weight * pixelE;
          // add error from pixelAdj
          outWeightsE[xIndex] += pixelESq * pixelESq;
        } else if (waveAdj) {
          auto waveY = waveAdj->y(0)[j];
          auto waveE = waveAdj->e(0)[j];

          outWeightsY[xIndex] += weight * waveY;
          const double waveESq = weight * waveE;
          // add error from waveAdj
          outWeightsE[xIndex] += waveESq * waveESq;
        } else
          outWeightsY[xIndex] += weight;
      }
    } // loop over single spectrum

    prog.report("Calculating Q");

  } // loop over all spectra

  // take sqrt of error weight values
  // left to be executed here for computational efficiency
  size_t numHist = weights->getNumberHistograms();
  for (size_t i = 0; i < numHist; i++) {
    auto &weightsE = weights->mutableE(i);
    std::transform(weightsE.cbegin(), weightsE.cend(), weightsE.begin(),
                   [&](double val) { return sqrt(val); });
  }

  bool doOutputParts = getProperty("OutputParts");
  if (doOutputParts) {
    // copy outputworkspace before it gets further modified
    MatrixWorkspace_sptr ws_sumOfCounts =
        WorkspaceFactory::Instance().create(outputWorkspace);
    for (size_t i = 0; i < ws_sumOfCounts->getNumberHistograms(); i++) {
      ws_sumOfCounts->setHistogram(i, outputWorkspace->histogram(i));
    }

    helper.outputParts(this, ws_sumOfCounts, weights);
  }

  // Divide the output data by the solid angles
  outputWorkspace /= weights;
  outputWorkspace->setDistribution(true);

  // Count of the number of empty cells
  const size_t nhist = outputWorkspace->getNumberHistograms();
  const size_t nbins = outputWorkspace->blocksize();
  int emptyBins = 0;
  for (size_t i = 0; i < nhist; ++i) {
    auto &yOut = outputWorkspace->y(i);
    for (size_t j = 0; j < nbins; ++j) {
      if (yOut[j] < 1.0e-12)
        ++emptyBins;
    }
  }

  // Log the number of empty bins
  g_log.notice() << "There are a total of " << emptyBins << " ("
                 << (100 * emptyBins) / (outputWorkspace->size())
                 << "%) empty Q bins.\n";
}

/** Creates the output workspace, setting the X vector to the bins boundaries in
 * Qx.
 *  @return A pointer to the newly-created workspace
 */
API::MatrixWorkspace_sptr
Qxy::setUpOutputWorkspace(API::MatrixWorkspace_const_sptr inputWorkspace) {
  const double max = getProperty("MaxQxy");
  const double delta = getProperty("DeltaQ");

  int bins = static_cast<int>(max / delta);
  if (bins * delta != max)
    ++bins; // Stop at first boundary past MaxQxy if max is not a multiple of
            // delta
  const double startVal = -1.0 * delta * bins;
  bins *= 2; // go from -max to +max
  bins += 1; // Add 1 - this is a histogram

  // Create an output workspace with the same meta-data as the input
  MatrixWorkspace_sptr outputWorkspace = WorkspaceFactory::Instance().create(
      inputWorkspace, bins - 1, bins, bins - 1);
  // ... but clear the masking from the parameter map as we don't want to carry
  // that over since this is essentially
  // a 2D rebin
  ParameterMap &pmap = outputWorkspace->instrumentParameters();
  pmap.clearParametersByName("masked");

  // Create a numeric axis to replace the vertical one
  Axis *verticalAxis = new BinEdgeAxis(bins);
  outputWorkspace->replaceAxis(1, verticalAxis);

  // Build up the X values
  HistogramData::BinEdges axis(bins,
                               HistogramData::LinearGenerator(startVal, delta));
  for (int i = 0; i < bins; ++i) {
    const double currentVal = startVal + i * delta;
    // Set the Y value on the axis
    verticalAxis->setValue(i, currentVal);
  }

  // Fill the X vectors in the output workspace
  for (int i = 0; i < bins - 1; ++i) {
    outputWorkspace->setBinEdges(i, axis);
    auto &y = outputWorkspace->mutableY(i);
    auto &e = outputWorkspace->mutableE(i);

    for (int j = 0; j < bins - j; ++j) {
      y[j] = std::numeric_limits<double>::quiet_NaN();
      e[j] = std::numeric_limits<double>::quiet_NaN();
    }
  }

  // Set the axis units
  outputWorkspace->getAxis(1)->unit() = outputWorkspace->getAxis(0)->unit() =
      UnitFactory::Instance().create("MomentumTransfer");
  // Set the 'Y' unit (gets confusing here...this is probably a Z axis in this
  // case)
  outputWorkspace->setYUnitLabel("Cross Section (1/cm)");

  setProperty("OutputWorkspace", outputWorkspace);
  return outputWorkspace;
}

} // namespace Algorithms
} // namespace Mantid
