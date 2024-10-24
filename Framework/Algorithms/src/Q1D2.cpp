// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <utility>

#include "MantidAPI/Axis.h"
#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/ISpectrum.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidAlgorithms/GravitySANSHelper.h"
#include "MantidAlgorithms/Q1D2.h"
#include "MantidAlgorithms/Qhelper.h"
#include "MantidDataObjects/Histogram1D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidTypes/SpectrumDefinition.h"

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(Q1D2)

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

Q1D2::Q1D2() : API::Algorithm(), m_dataWS(), m_doSolidAngle(false) {}

void Q1D2::init() {
  auto dataVal = std::make_shared<CompositeValidator>();
  dataVal->add<WorkspaceUnitValidator>("Wavelength");
  dataVal->add<HistogramValidator>();
  dataVal->add<InstrumentValidator>();
  dataVal->add<CommonBinsValidator>();
  declareProperty(std::make_unique<WorkspaceProperty<>>("DetBankWorkspace", "", Direction::Input, dataVal),
                  "Particle counts as a function of wavelength");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "Name of the workspace that will contain the result of the calculation");
  declareProperty(std::make_unique<ArrayProperty<double>>("OutputBinning", std::make_shared<RebinParamsValidator>()),
                  "A comma separated list of first bin boundary, width, last bin boundary. "
                  "Optionally\n"
                  "this can be followed by a comma and more widths and last boundary "
                  "pairs.\n"
                  "Negative width values indicate logarithmic binning.");
  declareProperty(std::make_unique<WorkspaceProperty<>>("PixelAdj", "", Direction::Input, PropertyMode::Optional),
                  "Scaling to apply to each spectrum. Must have\n"
                  "the same number of spectra as the DetBankWorkspace");
  auto wavVal = std::make_shared<CompositeValidator>();
  wavVal->add<WorkspaceUnitValidator>("Wavelength");
  wavVal->add<HistogramValidator>();
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("WavelengthAdj", "", Direction::Input, PropertyMode::Optional, wavVal),
      "Scaling to apply to each bin.\n"
      "Must have the same number of bins as the DetBankWorkspace");
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("WavePixelAdj", "", Direction::Input, PropertyMode::Optional, dataVal),
      "Scaling that depends on both pixel and wavelength together.\n"
      "Must have the same number of bins and spectra as the DetBankWorkspace.");
  declareProperty("AccountForGravity", false, "Whether to correct for the effects of gravity");
  declareProperty("SolidAngleWeighting", true, "If true, pixels will be weighted by their solid angle.",
                  Direction::Input);
  auto mustBePositive = std::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);

  declareProperty("RadiusCut", 0.0, mustBePositive,
                  "To increase resolution some wavelengths are excluded within "
                  "this distance from the beam center (mm). Note that RadiusCut\n"
                  " and WaveCut both need to be larger than 0 to affect \n"
                  "the effective cutoff. See the algorithm description for\n"
                  " a detailed explanation of the cutoff.");

  declareProperty("WaveCut", 0.0, mustBePositive,
                  "To increase resolution by starting to remove some wavelengths below this"
                  " threshold (angstrom).  Note that WaveCut\n"
                  " and RadiusCut both need to be larger than 0 to affect \n"
                  "on the effective cutoff. See the algorithm description for\n"
                  " a detailed explanation of the cutoff.");

  declareProperty("OutputParts", false,
                  "Set to true to output two additional workspaces which will "
                  "have the names OutputWorkspace_sumOfCounts "
                  "OutputWorkspace_sumOfNormFactors. The division of "
                  "_sumOfCounts and _sumOfNormFactors equals the workspace"
                  " returned by the property OutputWorkspace "
                  "(default is false).");
  declareProperty("ExtraLength", 0.0, mustBePositive, "Additional length for gravity correction.");

  declareProperty(
      std::make_unique<WorkspaceProperty<>>("QResolution", "", Direction::Input, PropertyMode::Optional, dataVal),
      "Workspace to calculate the Q resolution.\n");
}
/**
  @ throw invalid_argument if the workspaces are not mututially compatible
*/
void Q1D2::exec() {
  m_dataWS = getProperty("DetBankWorkspace");
  MatrixWorkspace_const_sptr waveAdj = getProperty("WavelengthAdj");
  MatrixWorkspace_const_sptr pixelAdj = getProperty("PixelAdj");
  MatrixWorkspace_const_sptr wavePixelAdj = getProperty("WavePixelAdj");
  MatrixWorkspace_const_sptr qResolution = getProperty("QResolution");

  const bool doGravity = getProperty("AccountForGravity");
  m_doSolidAngle = getProperty("SolidAngleWeighting");

  // throws if we don't have common binning or another incompatibility
  Qhelper helper;
  helper.examineInput(m_dataWS, waveAdj, pixelAdj, qResolution);
  // FIXME: how to examine the wavePixelAdj?
  g_log.debug() << "All input workspaces were found to be valid\n";
  // normalization as a function of wavelength (i.e. centers of x-value bins)
  double const *const binNorms = waveAdj ? &(waveAdj->y(0)[0]) : nullptr;
  // error on the wavelength normalization
  double const *const binNormEs = waveAdj ? &(waveAdj->e(0)[0]) : nullptr;

  // define the (large number of) data objects that are going to be used in all
  // iterations of the loop below

  // Flag to decide if Q Resolution is to be used
  auto useQResolution = static_cast<bool>(qResolution);

  // this will become the output workspace from this algorithm
  MatrixWorkspace_sptr outputWS = setUpOutputWorkspace(getProperty("OutputBinning"));

  auto &QOut = outputWS->x(0);
  auto &YOut = outputWS->mutableY(0);
  auto &EOutTo2 = outputWS->mutableE(0);
  // normalisation that is applied to counts in each Q bin
  HistogramData::HistogramY normSum(YOut.size(), 0.0);
  // the error on the normalisation
  HistogramData::HistogramE normError2(EOutTo2.size(), 0.0);
  // the averaged Q resolution.
  HistogramData::HistogramDx qResolutionOut(YOut.size(), 0.0);

  const auto numSpec = static_cast<int>(m_dataWS->getNumberHistograms());
  Progress progress(this, 0.05, 1.0, numSpec + 1);

  const auto &spectrumInfo = m_dataWS->spectrumInfo();
  PARALLEL_FOR_IF(Kernel::threadSafe(*m_dataWS, *outputWS, pixelAdj.get()))
  for (int i = 0; i < numSpec; ++i) {
    PARALLEL_START_INTERRUPT_REGION
    if (!spectrumInfo.hasDetectors(i)) {
      g_log.warning() << "Workspace index " << i << " (SpectrumIndex = " << m_dataWS->getSpectrum(i).getSpectrumNo()
                      << ") has no detector assigned to it - discarding\n";
      continue;
    }
    // Skip if we have a monitor or if the detector is masked.
    if (spectrumInfo.isMonitor(i) || spectrumInfo.isMasked(i))
      continue;

    // get the bins that are included inside the RadiusCut/WaveCutcut off, those
    // to calculate for
    // const size_t wavStart = waveLengthCutOff(i);
    const size_t wavStart =
        helper.waveLengthCutOff(m_dataWS, spectrumInfo, getProperty("RadiusCut"), getProperty("WaveCut"), i);
    if (wavStart >= m_dataWS->y(i).size()) {
      // all the spectra in this detector are out of range
      continue;
    }

    const size_t numWavbins = m_dataWS->y(i).size() - wavStart;
    // make just one call to new to reduce CPU overhead on each thread, access
    // to these
    // three "arrays" is via iterators
    HistogramData::HistogramY _noDirectUseStorage_(3 * numWavbins);
    // normalization term
    auto norms = _noDirectUseStorage_.begin();
    // the error on these weights, it contributes to the error calculation on
    // the output workspace
    auto normETo2s = norms + numWavbins;
    // the Q values calculated from input wavelength workspace
    auto QIn = normETo2s + numWavbins;

    // the weighting for this input spectrum that is added to the normalization
    calculateNormalization(wavStart, i, pixelAdj, wavePixelAdj, binNorms, binNormEs, norms, normETo2s);

    // now read the data from the input workspace, calculate Q for each bin
    convertWavetoQ(spectrumInfo, i, doGravity, wavStart, QIn, getProperty("ExtraLength"));

    // Pointers to the counts data and it's error
    auto YIn = m_dataWS->y(i).cbegin() + wavStart;
    auto EIn = m_dataWS->e(i).cbegin() + wavStart;

    // Pointers to the QResolution data. Note that the xdata was initially the
    // same, hence
    // the same indexing applies to the y values of m_dataWS and qResolution
    // If we want to use it set it to the correct value, else to YIN, although
    // that does not matter, as
    // we won't use it
    auto QResIn = useQResolution ? (qResolution->y(i).cbegin() + wavStart) : YIn;

    // when finding the output Q bin remember that the input Q bins (from the
    // convert to wavelength) start high and reduce
    auto loc = QOut.cend();
    // sum the Q contributions from each individual spectrum into the output
    // array
    const auto end = m_dataWS->y(i).cend();
    for (; YIn != end; ++YIn, ++EIn, ++QIn, ++norms, ++normETo2s) {
      // find the output bin that each input y-value will fall into, remembering
      // there is one more bin boundary than bins
      getQBinPlus1(QOut, *QIn, loc);
      // ignore counts that are out of the output range
      if ((loc != QOut.begin()) && (loc != QOut.end())) {
        // the actual Q-bin to add something to
        const size_t bin = loc - QOut.begin() - 1;
        PARALLEL_CRITICAL(q1d_counts_sum) {
          YOut[bin] += *YIn;
          normSum[bin] += *norms;
          // these are the errors squared which will be summed and square rooted
          // at the end
          EOutTo2[bin] += (*EIn) * (*EIn);
          normError2[bin] += *normETo2s;
          if (useQResolution) {
            auto QBin = (QOut[bin + 1] - QOut[bin]);
            // Here we need to take into account the Bin width and the count
            // weigthing. The
            // formula should be YIN* sqrt(QResIn^2 + (QBin/sqrt(12))^2)
            qResolutionOut[bin] += (*YIn) * std::sqrt((*QResIn) * (*QResIn) + QBin * QBin / 12.0);
          }
        }
      }

      // Increment the QResolution iterator
      if (useQResolution) {
        ++QResIn;
      }
    }

    PARALLEL_CRITICAL(q1d_spectra_map) {
      progress.report("Computing I(Q)");
      // Add up the detector IDs in the output spectrum at workspace index 0
      const auto &inSpec = m_dataWS->getSpectrum(i);
      auto &outSpec = outputWS->getSpectrum(0);
      outSpec.addDetectorIDs(inSpec.getDetectorIDs());
    }

    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  if (useQResolution) {
    // The number of Q (x)_ values is N, while the number of DeltaQ values is
    // N-1,
    // Richard Heenan suggested to duplicate the last entry of DeltaQ
    auto countsIterator = YOut.cbegin();
    auto qResolutionIterator = qResolutionOut.begin();
    for (; countsIterator != YOut.end(); ++countsIterator, ++qResolutionIterator) {
      // Divide by the counts of the Qbin, if the counts are 0, the the
      // qresolution will also be 0
      if ((*countsIterator) > 0.0) {
        *qResolutionIterator = (*qResolutionIterator) / (*countsIterator);
      }
    }
    outputWS->setPointStandardDeviations(0, std::move(qResolutionOut));
  }

  bool doOutputParts = getProperty("OutputParts");
  if (doOutputParts) {
    MatrixWorkspace_sptr ws_sumOfCounts = WorkspaceFactory::Instance().create(outputWS);
    ws_sumOfCounts->setSharedX(0, outputWS->sharedX(0));
    // Copy now as YOut is modified in normalize
    ws_sumOfCounts->mutableY(0) = YOut;
    ws_sumOfCounts->setSharedDx(0, outputWS->sharedDx(0));
    ws_sumOfCounts->setFrequencyVariances(0, outputWS->e(0));

    MatrixWorkspace_sptr ws_sumOfNormFactors = WorkspaceFactory::Instance().create(outputWS);
    ws_sumOfNormFactors->setSharedX(0, outputWS->sharedX(0));
    ws_sumOfNormFactors->mutableY(0) = normSum;
    ws_sumOfNormFactors->setSharedDx(0, outputWS->sharedDx(0));
    ws_sumOfNormFactors->setFrequencyVariances(0, normError2);

    helper.outputParts(this, ws_sumOfCounts, ws_sumOfNormFactors);
  }

  progress.report("Normalizing I(Q)");
  // finally divide the number of counts in each output Q bin by its weighting
  normalize(normSum, normError2, YOut, EOutTo2);

  setProperty("OutputWorkspace", outputWS);
}

/** Creates the output workspace, its size, units, etc.
 *  @param binParams the bin boundary specification using the same same syntax
 * as param the Rebin algorithm
 *  @return A pointer to the newly-created workspace
 */
API::MatrixWorkspace_sptr Q1D2::setUpOutputWorkspace(const std::vector<double> &binParams) const {
  // Calculate the output binning
  HistogramData::BinEdges XOut(0);
  static_cast<void>(VectorHelper::createAxisFromRebinParams(binParams, XOut.mutableRawData()));

  // Create output workspace. On all but rank 0 this is a temporary workspace.
  Indexing::IndexInfo indexInfo(1);
  indexInfo.setSpectrumDefinitions(std::vector<SpectrumDefinition>(1));
  auto outputWS = create<MatrixWorkspace>(*m_dataWS, indexInfo, XOut);
  outputWS->getAxis(0)->unit() = UnitFactory::Instance().create("MomentumTransfer");
  outputWS->setYUnitLabel("1/cm");

  outputWS->setDistribution(true);

  outputWS->getSpectrum(0).clearDetectorIDs();
  outputWS->getSpectrum(0).setSpectrumNo(1);

  return outputWS;
}

/** Calculate the normalization term for each output bin
 *  @param wavStart [in] the index number of the first bin in the input
 * wavelengths that is actually being used
 *  @param wsIndex [in] the ws index of the spectrum to calculate
 *  @param pixelAdj [in] if not NULL this is workspace contains single bins with
 * the adjustments, e.g. detector efficencies, for the given ws index
 *  @param wavePixelAdj [in] if not NULL this is workspace that contains the
 * adjustments for the pixels and wavelenght dependend values.
 *  @param binNorms [in] pointer to a contigious array of doubles that are the
 * wavelength correction from waveAdj workspace, can be NULL
 *  @param binNormEs [in] pointer to a contigious array of doubles which
 * corrospond to the corrections and are their errors, can be NULL
 *  @param norm [out] normalization for each bin, including soild angle, pixel
 * correction, the proportion that is not masked and the normalization workspace
 *  @param normETo2 [out] this pointer must point to the end of the norm array,
 * it will be filled with the total of the error on the normalization
 */
void Q1D2::calculateNormalization(const size_t wavStart, const size_t wsIndex,
                                  const API::MatrixWorkspace_const_sptr &pixelAdj,
                                  const API::MatrixWorkspace_const_sptr &wavePixelAdj, double const *const binNorms,
                                  double const *const binNormEs, HistogramData::HistogramY::iterator norm,
                                  HistogramData::HistogramY::iterator normETo2) const {
  double detectorAdj, detAdjErr;
  pixelWeight(pixelAdj, wsIndex, detectorAdj, detAdjErr);
  // use that the normalization array ends at the start of the error array
  for (auto n = norm, e = normETo2; n != normETo2; ++n, ++e) {
    *n = detectorAdj;
    *e = detAdjErr * detAdjErr;
  }

  if (binNorms && binNormEs) {
    if (wavePixelAdj)
      // pass the iterator for the wave pixel Adj dependent
      addWaveAdj(binNorms + wavStart, binNormEs + wavStart, norm, normETo2, wavePixelAdj->y(wsIndex).begin() + wavStart,
                 wavePixelAdj->e(wsIndex).begin() + wavStart);
    else
      addWaveAdj(binNorms + wavStart, binNormEs + wavStart, norm, normETo2);
  }
  normToMask(wavStart, wsIndex, norm, normETo2);
}

/** Calculates the normalisation for the spectrum specified by the index number
 * that was passed
 *  as the solid angle multiplied by the pixelAdj that was passed
 *  @param[in] pixelAdj if not NULL this is workspace contains single bins with
 * the adjustments, e.g. detector efficiencies, for the given ws index
 *  @param[in] wsIndex the workspace index to return the data from
 *  @param[out] weight the solid angle or if pixelAdj the solid angle times the
 * pixel adjustment for this spectrum
 *  @param[out] error the error on the weight, only non-zero if pixelAdj
 *  @throw LogicError if the solid angle is tiny or negative
 */
void Q1D2::pixelWeight(const API::MatrixWorkspace_const_sptr &pixelAdj, const size_t wsIndex, double &weight,
                       double &error) const {
  const auto &detectorInfo = m_dataWS->detectorInfo();
  const V3D samplePos = detectorInfo.samplePosition();

  if (m_doSolidAngle) {
    weight = 0.0;
    for (const auto detID : m_dataWS->getSpectrum(wsIndex).getDetectorIDs()) {
      const auto index = detectorInfo.indexOf(detID);
      if (!detectorInfo.isMasked(index))
        weight += detectorInfo.detector(index).solidAngle(samplePos);
    }
  } else
    weight = 1.0;

  if (weight < 1e-200) {
    throw std::logic_error("Invalid (zero or negative) solid angle for one detector");
  }
  // this input multiplies up the adjustment if it exists
  if (pixelAdj) {
    weight *= pixelAdj->readY(wsIndex)[0];
    error = weight * pixelAdj->readE(wsIndex)[0];
  } else {
    error = 0.0;
  }
}

/** Calculates the contribution to the normalization terms from each bin in a
 * spectrum
 *  @param[in] c pointer to the start of a contigious array of wavelength
 * dependent normalization terms
 *  @param[in] Dc pointer to the start of a contigious array that corrosponds to
 * wavelength dependent term, having its error
 *  @param[in,out] bInOut normalization for each bin, this method multiplise
 * this by the proportion that is not masked and the normalization workspace
 *  @param[in, out] e2InOut this array must follow straight after the
 * normalization array and will contain the error on the normalisation term
 * before the WavelengthAdj term
 */
void Q1D2::addWaveAdj(const double *c, const double *Dc, HistogramData::HistogramY::iterator bInOut,
                      HistogramData::HistogramY::iterator e2InOut) const {
  // normalize by the wavelength dependent correction, keeping the percentage
  // errors the same
  // the error when a = b*c, the formula for Da, the error on a, in terms of Db,
  // etc. is (Da/a)^2 = (Db/b)^2 + (Dc/c)^2
  //(Da)^2 = ((Db*a/b)^2 + (Dc*a/c)^2) = (Db*c)^2 + (Dc*b)^2
  // the variable names relate to those above as: existing values (b=bInOut)
  // multiplied by the additional errors (Dc=binNormEs), existing errors
  // (Db=sqrt(e2InOut)) times new factor (c=binNorms)

  // use the fact that error array follows straight after the normalization
  // array
  const auto end = e2InOut;
  for (; bInOut != end; ++e2InOut, ++c, ++Dc, ++bInOut) {
    // first the error
    *e2InOut = ((*e2InOut) * (*c) * (*c)) + ((*Dc) * (*Dc) * (*bInOut) * (*bInOut));
    // now the actual calculation a = b*c
    *bInOut = (*bInOut) * (*c);
  }
}
/** Calculates the contribution to the normalization terms from each bin in a
 * spectrum
 *  @param[in] c pointer to the start of a contigious array of wavelength
 * dependent normalization terms
 *  @param[in] Dc pointer to the start of a contigious array that corrosponds to
 * wavelength dependent term, having its error
 *  @param[in,out] bInOut normalization for each bin, this method multiplise
 * this by the proportion that is not masked and the normalization workspace
 *  @param[in, out] e2InOut this array must follow straight after the
 * normalization array and will contain the error on the normalisation term
 * before the WavelengthAdj term
 * @param[in] wavePixelAdjData normalization correction for each bin for each
 * detector pixel.
 * @param[in] wavePixelAdjError normalization correction incertainty for each
 * bin for each detector pixel.
 */
void Q1D2::addWaveAdj(const double *c, const double *Dc, HistogramData::HistogramY::iterator bInOut,
                      HistogramData::HistogramY::iterator e2InOut,
                      HistogramData::HistogramY::const_iterator wavePixelAdjData,
                      HistogramData::HistogramE::const_iterator wavePixelAdjError) const {
  // normalize by the wavelength dependent correction, keeping the percentage
  // errors the same
  // the error when a = b*c*e, the formula for Da, the error on a, in terms of
  // Db, etc. is
  // (Da/a)^2 = (Db/b)^2 + (Dc/c)^2 + (De/e)^2
  //(Da)^2 = ((Db*a/b)^2 + (Dc*a/c)^2) + (De * a/e)^2
  // But: a/b = c*e; a/c = b*e; a/e = b*c;
  // So:
  // (Da)^2 = (c*e*Db)^2 + (b*e*Dc)^2 + (b*c*De)^2
  //
  // Consider:
  // Da = Error (e2InOut)
  // Db^2 = PixelDependentError (e2InOut)
  // b  = PixelDependentValue (bInOut)
  // c  = WaveDependentValue (c)
  // Dc = WaveDependentError (Dc)
  // e  = PixelWaveDependentValue (wavePixelAdjData)
  // De = PiexlWaveDependentError (wavePixelAdjError)

  // use the fact that error array follows straight after the normalization
  // array
  const auto end = e2InOut;
  for (; bInOut != end; ++e2InOut, ++c, ++Dc, ++bInOut, ++wavePixelAdjData, ++wavePixelAdjError) {
    // first the error
    *e2InOut = ((*e2InOut) * (*c) * (*c) * (*wavePixelAdjData) * (*wavePixelAdjData)) +
               ((*Dc) * (*Dc) * (*bInOut) * (*bInOut) * (*wavePixelAdjData) * (*wavePixelAdjData)) +
               ((*wavePixelAdjError) * (*wavePixelAdjError) * (*c) * (*c) * (*bInOut) * (*bInOut));
    // now the actual calculation a = b*c*e : Pixel * Wave * PixelWave
    *bInOut = (*bInOut) * (*c) * (*wavePixelAdjData);
  }
}

/** Scaled to bin masking, to the normalization
 *  @param[in] offSet the index number of the first bin in the input wavelengths
 * that is actually being used
 *  @param[in] wsIndex the spectrum to calculate
 *  @param[in,out] theNorms normalization for each bin, this is multiplied by
 * the proportion that is not masked and the normalization workspace
 *  @param[in,out] errorSquared the running total of the square of the
 * uncertainty in the normalization
 */
void Q1D2::normToMask(const size_t offSet, const size_t wsIndex, const HistogramData::HistogramY::iterator theNorms,
                      const HistogramData::HistogramY::iterator errorSquared) const {
  // if any bins are masked it is normally a small proportion
  if (m_dataWS->hasMaskedBins(wsIndex)) {
    // Get a reference to the list of masked bins
    const MatrixWorkspace::MaskList &mask = m_dataWS->maskedBins(wsIndex);
    // Now iterate over the list, adjusting the weights for the affected bins
    MatrixWorkspace::MaskList::const_iterator it;
    for (it = mask.begin(); it != mask.end(); ++it) {
      size_t outBin = it->first;
      if (outBin < offSet) {
        // this masked bin wasn't in the range being delt with anyway
        continue;
      }
      outBin -= offSet;
      // The weight for this masked bin is 1 - the degree to which this bin is
      // masked
      const double factor = 1.0 - (it->second);
      *(theNorms + outBin) *= factor;
      *(errorSquared + outBin) *= factor * factor;
    }
  }
}

/** Fills a vector with the Q values calculated from the wavelength bin centers
 * from the input workspace and
 *  the workspace geometry as Q = 4*pi*sin(theta)/lambda
 *  @param[in] spectrumInfo SpectrumInfo for workspace
 *  @param[in] wsInd the spectrum to calculate
 *  @param[in] doGravity if to include gravity in the calculation of Q
 *  @param[in] offset index number of the first input bin to use
 *  @param[in] extraLength for gravitational correction
 *  @param[out] Qs points to a preallocated array that is large enough to
 * contain all the calculated Q values
 *  @throw NotFoundError if the detector associated with the spectrum is not
 * found in the instrument definition
 */
void Q1D2::convertWavetoQ(const SpectrumInfo &spectrumInfo, const size_t wsInd, const bool doGravity,
                          const size_t offset, HistogramData::HistogramY::iterator Qs, const double extraLength) const {
  static const double FOUR_PI = 4.0 * M_PI;

  // wavelengths (lamda) to be converted to Q
  auto waves = m_dataWS->x(wsInd).cbegin() + offset;
  // going from bin boundaries to bin centered x-values the size goes down one
  const auto end = m_dataWS->x(wsInd).end() - 1;
  if (doGravity) {
    GravitySANSHelper grav(spectrumInfo, wsInd, extraLength);
    for (; waves != end; ++Qs, ++waves) {
      // the HistogramValidator at the start should ensure that we have one more
      // bin on the input wavelengths
      const double lambda = 0.5 * (*(waves + 1) + (*waves));
      // as the fall under gravity is wavelength dependent sin theta is now
      // different for each bin with each detector
      const double sinTheta = grav.calcSinTheta(lambda);
      // Now we're ready to go to Q
      *Qs = FOUR_PI * sinTheta / lambda;
    }
  } else {
    // Calculate the Q values for the current spectrum, using Q =
    // 4*pi*sin(theta)/lambda
    const double factor = 2.0 * FOUR_PI * sin(spectrumInfo.twoTheta(wsInd) * 0.5);
    for (; waves != end; ++Qs, ++waves) {
      // the HistogramValidator at the start should ensure that we have one more
      // bin on the input wavelengths
      *Qs = factor / (*(waves + 1) + (*waves));
    }
  }
}
/** This is a slightly "clever" method as it makes some guesses about where is
 * best
 *  to look for the right Q bin based on the fact that the input Qs (calcualted
 * from wavelengths) tend
 *  to go down while the output Qs are always in accending order
 *  @param[in] OutQs the array of output Q bin boundaries, this finds the bin
 * that contains the QIn value
 *  @param[in] QToFind the Q value to find the correct bin for
 *  @param[in, out] loc points to the bin boundary (in the OutQs array) whos Q
 * is higher than QToFind and higher by the smallest amount. Algorithm starts by
 * checking the value of loc passed and then all the bins _downwards_ through
 * the array
 */
void Q1D2::getQBinPlus1(const HistogramData::HistogramX &OutQs, const double QToFind,
                        HistogramData::HistogramX::const_iterator &loc) const {
  if (loc != OutQs.end()) {
    while (loc != OutQs.begin()) {
      if ((QToFind >= *(loc - 1)) && (QToFind < *loc)) {
        return;
      }
      --loc;
    }
    if (QToFind < *loc) {
      // QToFind is outside the array leave loc == OutQs.begin()
      return;
    }
  } else // loc == OutQs.end()
  {
    if (OutQs.empty() || QToFind > *(loc - 1)) {
      // outside the array leave loc == OutQs.end()
      return;
    }
  }

  // we are lost, normally the order of the Q values means we only get here on
  // the first iteration. It's slow
  loc = std::lower_bound(OutQs.begin(), OutQs.end(), QToFind);
}

/** Divides the number of counts in each output Q bin by the wrighting ("number
 * that would expected to arrive")
 *  The errors are propogated using the uncorrolated error estimate for
 * multiplication/division
 *  @param[in] normSum the weighting for each bin
 *  @param[in] normError2 square of the error on the normalization
 *  @param[in, out] counts counts in each bin
 *  @param[in, out] errors input the _square_ of the error on each bin, output
 * the total error (unsquared)
 */
void Q1D2::normalize(const HistogramData::HistogramY &normSum, const HistogramData::HistogramE &normError2,
                     HistogramData::HistogramY &counts, HistogramData::HistogramE &errors) const {
  for (size_t k = 0; k < counts.size(); ++k) {
    // the normalisation is a = b/c where b = counts c =normalistion term
    const double c = normSum[k];
    const double a = counts[k] /= c;
    // when a = b/c, the formula for Da, the error on a, in terms of Db, etc. is
    // (Da/a)^2 = (Db/b)^2 + (Dc/c)^2
    //(Da)^2 = ((Db/b)^2 + (Dc/c)^2)*(b^2/c^2) = ((Db/c)^2 + (b*Dc/c^2)^2) =
    //(Db^2 + (b*Dc/c)^2)/c^2 = (Db^2 + (Dc*a)^2)/c^2
    // this will work as long as c>0, but then the above formula above can't
    // deal with 0 either
    const double aOverc = a / c;
    errors[k] = std::sqrt(errors[k] / (c * c) + normError2[k] * aOverc * aOverc);
  }
}
} // namespace Mantid::Algorithms
