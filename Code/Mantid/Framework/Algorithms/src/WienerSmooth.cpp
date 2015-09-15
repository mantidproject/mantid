#include "MantidAlgorithms/WienerSmooth.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/TextAxis.h"
#include "MantidKernel/ArrayProperty.h"

#include <numeric>

namespace Mantid {
namespace Algorithms {

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(WienerSmooth)

namespace {
// Square values
struct PowerSpectrum {
  double operator()(double x) const { return x * x; }
};

// To be used when actual noise level cannot be estimated
const double guessSignalToNoiseRatio = 1e15;
}

//----------------------------------------------------------------------------------------------
/** Constructor
 */
WienerSmooth::WienerSmooth() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
WienerSmooth::~WienerSmooth() {}

//----------------------------------------------------------------------------------------------

/// Algorithm's version for identification. @see Algorithm::version
int WienerSmooth::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string WienerSmooth::category() const {
  return "Arithmetic\\FFT;Transforms\\Smoothing";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string WienerSmooth::summary() const {
  return "Smooth spectra using Wiener filter.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void WienerSmooth::init() {
  declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
      "An input workspace.");
  declareProperty(new Kernel::ArrayProperty<int>("WorkspaceIndexList"),
                  "Workspace indices for spectra to process. "
                  "If empty smooth all spectra.");
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void WienerSmooth::exec() {
  // Get the data to smooth
  API::MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  std::vector<int> wsIndexList = getProperty("WorkspaceIndexList");

  // number of spectra in the input workspace
  const size_t nInputSpectra = inputWS->getNumberHistograms();

  // Validate the input
  if (wsIndexList.size() > nInputSpectra) {
    throw std::invalid_argument("Workspace index list has more indices than "
                                "there are spectra in the input workspace.");
  }

  // if empty do whole workspace
  if (wsIndexList.empty()) {
    // fill wsIndexList with consecutive integers from 0 to nSpectra - 1
    wsIndexList.resize(nInputSpectra);
    wsIndexList.front() = 0;
    for (auto index = wsIndexList.begin() + 1; index != wsIndexList.end();
         ++index) {
      *index = *(index - 1) + 1;
    }
  }

  // number of spectra in the output workspace
  const size_t nOutputSpectra = wsIndexList.size();

  // smooth the first spectrum to find out the output blocksize
  size_t wsIndex = static_cast<size_t>(wsIndexList.front());
  auto first = smoothSingleSpectrum(inputWS, wsIndex);

  // create full output workspace by copying all settings from tinputWS
  // blocksize is taken form first
  API::MatrixWorkspace_sptr outputWS = API::WorkspaceFactory::Instance().create(
      inputWS, nOutputSpectra, first->readX(0).size(), first->readY(0).size());

  // TODO: ideally axis cloning should be done via API::Axis interface but it's
  // not possible
  // at he moment and as it turned out not straight-forward to implement
  auto inAxis = inputWS->getAxis(1);
  auto outAxis = inAxis->clone(nOutputSpectra, outputWS.get());
  outputWS->replaceAxis(1, outAxis);

  bool isSpectra = outAxis->isSpectra();
  bool isNumeric = outAxis->isNumeric();
  auto inTextAxis = dynamic_cast<API::TextAxis *>(inAxis);
  auto outTextAxis = dynamic_cast<API::TextAxis *>(outAxis);

  // Initialise the progress reporting object
  API::Progress progress(this, 0.0, 1.0, nOutputSpectra);

  // smooth the rest of the input
  for (size_t outIndex = 0; outIndex < nOutputSpectra; ++outIndex) {
    auto inIndex = wsIndexList[outIndex];
    auto next = outIndex == 0 ? first : smoothSingleSpectrum(inputWS, inIndex);

    // copy the values
    outputWS->dataX(outIndex) = next->readX(0);
    outputWS->dataY(outIndex) = next->readY(0);
    outputWS->dataE(outIndex) = next->readE(0);

    // set the axis value
    if (isSpectra) {
      auto inSpectrum = inputWS->getSpectrum(inIndex);
      auto outSpectrum = outputWS->getSpectrum(outIndex);
      outSpectrum->setSpectrumNo(inSpectrum->getSpectrumNo());
      outSpectrum->setDetectorIDs(inSpectrum->getDetectorIDs());
    } else if (isNumeric) {
      outAxis->setValue(outIndex, inAxis->getValue(inIndex));
    } else if (inTextAxis && outTextAxis) {
      outTextAxis->setLabel(outIndex, inTextAxis->label(inIndex));
    }
    progress.report();
  }

  // set the output
  setProperty("OutputWorkspace", outputWS);
}

//----------------------------------------------------------------------------------------------
/**
 * Execute smoothing of a single spectrum.
 * @param inputWS :: A workspace to pick a spectrum from.
 * @param wsIndex :: An index of a spectrum to smooth.
 * @return :: A single-spectrum workspace with the smoothed data.
 */
API::MatrixWorkspace_sptr
WienerSmooth::smoothSingleSpectrum(API::MatrixWorkspace_sptr inputWS,
                                   size_t wsIndex) {
  size_t dataSize = inputWS->blocksize();

  // it won't work for very small workspaces
  if (dataSize < 4) {
    g_log.debug() << "No smoothing, spectrum copied." << std::endl;
    return copyInput(inputWS, wsIndex);
  }

  // Due to the way RealFFT works the input should be even-sized
  const bool isOddSize = dataSize % 2 != 0;
  if (isOddSize) {
    // add a fake value to the end to make size even
    inputWS = copyInput(inputWS, wsIndex);
    wsIndex = 0;
    auto &X = inputWS->dataX(wsIndex);
    auto &Y = inputWS->dataY(wsIndex);
    auto &E = inputWS->dataE(wsIndex);
    double dx = X[dataSize - 1] - X[dataSize - 2];
    X.push_back(X.back() + dx);
    Y.push_back(Y.back());
    E.push_back(E.back());
  }

  // the input vectors
  auto &X = inputWS->readX(wsIndex);
  auto &Y = inputWS->readY(wsIndex);
  auto &E = inputWS->readE(wsIndex);

  // Digital fourier transform works best for data oscillating around 0.
  // Fit a spline with a small number of break points to the data.
  // Make sure that the spline passes through the first and the last points
  // of the data.
  // The fitted spline will be subtracted from the data and the difference
  // will be smoothed with the Wiener filter. After that the spline will be
  // added to the smoothed data to produce the output.

  // number of spline break points, must be smaller than the data size but
  // between 2 and 10
  size_t nbreak = 10;
  if (nbreak * 3 > dataSize)
    nbreak = dataSize / 3;

  // NB. The spline mustn't fit too well to the data. If it does smoothing
  // doesn't happen.
  // TODO: it's possible that the spline is unnecessary and a simple linear
  // function will
  //       do a better job.

  g_log.debug() << "Spline break points " << nbreak << std::endl;

  // define the spline
  API::IFunction_sptr spline =
      API::FunctionFactory::Instance().createFunction("BSpline");
  auto xInterval = getStartEnd(X, inputWS->isHistogramData());
  spline->setAttributeValue("StartX", xInterval.first);
  spline->setAttributeValue("EndX", xInterval.second);
  spline->setAttributeValue("NBreak", static_cast<int>(nbreak));
  // fix the first and last parameters to the first and last data values
  spline->setParameter(0, Y.front());
  spline->fix(0);
  size_t lastParamIndex = spline->nParams() - 1;
  spline->setParameter(lastParamIndex, Y.back());
  spline->fix(lastParamIndex);

  // fit the spline to the data
  auto fit = createChildAlgorithm("Fit");
  fit->initialize();
  fit->setProperty("Function", spline);
  fit->setProperty("InputWorkspace", inputWS);
  fit->setProperty("WorkspaceIndex", static_cast<int>(wsIndex));
  fit->setProperty("CreateOutput", true);
  fit->execute();

  // get the fit output workspace; spectrum 2 contains the difference that is to
  // be smoothed
  API::MatrixWorkspace_sptr fitOut = fit->getProperty("OutputWorkspace");

  // Fourier transform the difference spectrum
  auto fourier = createChildAlgorithm("RealFFT");
  fourier->initialize();
  fourier->setProperty("InputWorkspace", fitOut);
  fourier->setProperty("WorkspaceIndex", 2);
  // we don't require bin linearity as we don't need the exact transform
  fourier->setProperty("IgnoreXBins", true);
  fourier->execute();

  API::MatrixWorkspace_sptr fourierOut =
      fourier->getProperty("OutputWorkspace");

  // spectrum 2 of the transformed workspace has the transform modulus which is
  // a square
  // root of the power spectrum
  auto &powerSpec = fourierOut->dataY(2);
  // convert the modulus to power spectrum wich is the base of the Wiener filter
  std::transform(powerSpec.begin(), powerSpec.end(), powerSpec.begin(),
                 PowerSpectrum());

  // estimate power spectrum's noise as the average of its high frequency half
  size_t n2 = powerSpec.size();
  double noise =
      std::accumulate(powerSpec.begin() + n2 / 2, powerSpec.end(), 0.0);
  noise /= static_cast<double>(n2);

  // index of the maximum element in powerSpec
  const size_t imax = static_cast<size_t>(std::distance(
      powerSpec.begin(), std::max_element(powerSpec.begin(), powerSpec.end())));

  if (noise == 0.0) {
    noise = powerSpec[imax] / guessSignalToNoiseRatio;
  }

  g_log.debug() << "Maximum signal " << powerSpec[imax] << std::endl;
  g_log.debug() << "Noise          " << noise << std::endl;

  // storage for the Wiener filter, initialized with 0.0's
  std::vector<double> wf(n2);

  // The filter consists of two parts:
  //   1) low frequency region, from 0 until the power spectrum falls to the
  //   noise level, filter is calculated
  //      from the power spectrum
  //   2) high frequency noisy region, filter is a smooth function of frequency
  //   decreasing to 0

  // the following code is an adaptation of a fortran routine
  // noise starting index
  size_t i0 = 0;
  // intermediate variables
  double xx = 0.0;
  double xy = 0.0;
  double ym = 0.0;
  // low frequency filter values: the higher the power spectrum the closer the
  // filter to 1.0
  for (size_t i = 0; i < n2; ++i) {
    double cd1 = powerSpec[i] / noise;
    if (cd1 < 1.0 && i > imax) {
      i0 = i;
      break;
    }
    double cd2 = log(cd1);
    wf[i] = cd1 / (1.0 + cd1);
    double j = static_cast<double>(i + 1);
    xx += j * j;
    xy += j * cd2;
    ym += cd2;
  }

  // i0 should always be > 0 but in case something goes wrong make a check
  if (i0 > 0) {
    g_log.debug() << "Noise start index " << i0 << std::endl;

    // high frequency filter values: smooth decreasing function
    double ri0f = static_cast<double>(i0 + 1);
    double xm = (1.0 + ri0f) / 2;
    ym /= ri0f;
    double a1 = (xy - ri0f * xm * ym) / (xx - ri0f * xm * xm);
    double b1 = ym - a1 * xm;

    g_log.debug() << "(a1,b1) = (" << a1 << ',' << b1 << ')' << std::endl;

    const double dblev = -20.0;
    // cut-off index
    double ri1 = floor((dblev / 4 - b1) / a1);
    if (ri1 < static_cast<double>(i0)) {
      g_log.warning() << "Failed to build Wiener filter: no smoothing."
                      << std::endl;
      ri1 = static_cast<double>(i0);
    }
    size_t i1 = static_cast<size_t>(ri1);
    if (i1 > n2)
      i1 = n2;
    for (size_t i = i0; i < i1; ++i) {
      double s = exp(a1 * static_cast<double>(i + 1) + b1);
      wf[i] = s / (1.0 + s);
    }
    // wf[i] for i1 <= i < n2 are 0.0

    g_log.debug() << "Cut-off index " << i1 << std::endl;
  } else {
    g_log.warning() << "Power spectrum has an unexpected shape: no smoothing"
                    << std::endl;
    return copyInput(inputWS, wsIndex);
  }

  // multiply the fourier transform by the filter
  auto &re = fourierOut->dataY(0);
  auto &im = fourierOut->dataY(1);

  std::transform(re.begin(), re.end(), wf.begin(), re.begin(),
                 std::multiplies<double>());
  std::transform(im.begin(), im.end(), wf.begin(), im.begin(),
                 std::multiplies<double>());

  // inverse fourier transform
  fourier = createChildAlgorithm("RealFFT");
  fourier->initialize();
  fourier->setProperty("InputWorkspace", fourierOut);
  fourier->setProperty("IgnoreXBins", true);
  fourier->setPropertyValue("Transform", "Backward");
  fourier->execute();

  API::MatrixWorkspace_sptr out = fourier->getProperty("OutputWorkspace");
  auto &background = fitOut->readY(1);
  auto &y = out->dataY(0);

  if (y.size() != background.size()) {
    throw std::logic_error("Logic error: inconsistent arrays");
  }

  // add the spline "background" to the smoothed data
  std::transform(y.begin(), y.end(), background.begin(), y.begin(),
                 std::plus<double>());

  // copy the x-values and errors from the original spectrum
  // remove the last values for odd-sized inputs
  if (isOddSize) {
    out->dataX(0).assign(X.begin(), X.end() - 1);
    out->dataE(0).assign(E.begin(), E.end() - 1);
    out->dataY(0).resize(Y.size() - 1);
  } else {
    out->setX(0, X);
    out->dataE(0).assign(E.begin(), E.end());
  }

  return out;
}

/**
 * Get the start and end of the x-interval.
 * @param X :: The x-vector of a spectrum.
 * @param isHistogram :: Is the x-vector comming form a histogram? If it's true
 * the bin
 *   centres are used.
 * @return :: A pair of start x and end x.
 */
std::pair<double, double> WienerSmooth::getStartEnd(const MantidVec &X,
                                                    bool isHistogram) const {
  const size_t n = X.size();
  if (n < 3) {
    // 3 is the smallest number for this method to work without breaking
    throw std::runtime_error(
        "Number of bins/data points cannot be smaller than 3.");
  }
  if (isHistogram) {
    return std::make_pair((X[0] + X[1]) / 2, (X[n - 1] + X[n - 2]) / 2);
  }
  // else
  return std::make_pair(X.front(), X.back());
}

/**
 * Exctract the input spectrum into a separate workspace.
 * @param inputWS :: The input workspace.
 * @param wsIndex :: The index of the input spectrum.
 * @return :: Workspace with the copied spectrum.
 */
API::MatrixWorkspace_sptr
WienerSmooth::copyInput(API::MatrixWorkspace_sptr inputWS, size_t wsIndex) {
  auto alg = createChildAlgorithm("ExtractSingleSpectrum");
  alg->initialize();
  alg->setProperty("InputWorkspace", inputWS);
  alg->setProperty("WorkspaceIndex", static_cast<int>(wsIndex));
  alg->execute();
  API::MatrixWorkspace_sptr ws = alg->getProperty("OutputWorkspace");
  return ws;
}

} // namespace Algorithms
} // namespace Mantid