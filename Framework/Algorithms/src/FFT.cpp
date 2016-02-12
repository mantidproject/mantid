//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FFT.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/UnitLabelTypes.h"
#include "MantidAPI/TextAxis.h"

#include <boost/shared_array.hpp>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_fft_complex.h>

#define REAL(z, i) ((z)[2 * (i)])
#define IMAG(z, i) ((z)[2 * (i) + 1])

#include <sstream>
#include <numeric>
#include <algorithm>
#include <functional>
#include <cmath>
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid {
namespace Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(FFT)

using namespace Kernel;
using namespace API;

/// Initialisation method. Declares properties to be used in algorithm.
void FFT::init() {
  declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
      "The name of the input workspace.");
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "The name of the output workspace.");
  // if desired, provide the imaginary part in a separate workspace.
  declareProperty(new WorkspaceProperty<>("InputImagWorkspace", "",
                                          Direction::Input,
                                          PropertyMode::Optional),
                  "The name of the input workspace for the imaginary part. "
                  "Leave blank if same as InputWorkspace");

  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty("Real", 0, mustBePositive,
                  "Spectrum number to use as real part for transform");
  declareProperty("Imaginary", EMPTY_INT(), mustBePositive,
                  "Spectrum number to use as imaginary part for transform");

  std::vector<std::string> fft_dir{"Forward", "Backward"};
  declareProperty("Transform", "Forward",
                  boost::make_shared<StringListValidator>(fft_dir),
                  "Direction of the transform: forward or backward");
  declareProperty("Shift", 0.0, "Apply an extra phase equal to this quantity "
                                "times 2*pi to the transform");
  declareProperty(
      "AcceptXRoundingErrors", false,
      "Continue to process the data even if X values are not evenly spaced",
      Direction::Input);
}

/** Executes the algorithm
 *
 *  @throw std::invalid_argument if the input properties are invalid
                                 or the bins of the spectrum being transformed
 do not have constant width
 */
void FFT::exec() {
  MatrixWorkspace_const_sptr inWS = getProperty("InputWorkspace");
  MatrixWorkspace_const_sptr inImagWS = getProperty("InputImagWorkspace");
  if (!inImagWS)
    inImagWS = inWS; // workspaces are one and the same

  const int iReal = getProperty("Real");
  const int iImag = getProperty("Imaginary");
  const bool isComplex = iImag != EMPTY_INT();

  const MantidVec &X = inWS->readX(iReal);
  const int ySize = static_cast<int>(inWS->blocksize());
  const int xSize = static_cast<int>(X.size());

  int nHist = static_cast<int>(inWS->getNumberHistograms());
  if (iReal >= nHist)
    throw std::invalid_argument("Property Real is out of range");
  if (isComplex) {
    const int yImagSize = static_cast<int>(inImagWS->blocksize());
    if (ySize != yImagSize)
      throw std::length_error("Real and Imaginary sizes do not match");
    nHist = static_cast<int>(inImagWS->getNumberHistograms());
    if (iImag >= nHist)
      throw std::invalid_argument("Property Imaginary is out of range");
  }

  gsl_fft_complex_wavetable *wavetable = gsl_fft_complex_wavetable_alloc(ySize);
  gsl_fft_complex_workspace *workspace = gsl_fft_complex_workspace_alloc(ySize);

  boost::shared_array<double> data(new double[2 * ySize]);
  const std::string transform = getProperty("Transform");

  // The number of spectra in the output workspace
  int nOut = 3;
  bool addPositiveOnly = false;
  // If the input is real add 3 more spectra with positive "frequencies" only
  if (!isComplex && transform == "Forward") {
    nOut += 3;
    addPositiveOnly = true;
  }

  MatrixWorkspace_sptr outWS =
      WorkspaceFactory::Instance().create(inWS, nOut, xSize, ySize);

  const double dx = X[1] - X[0];
  double df = 1.0 / (dx * ySize);

  // Output label
  outWS->getAxis(0)->unit() = UnitFactory::Instance().create("Label");

  auto inputUnit = inWS->getAxis(0)->unit();
  if (inputUnit) {

    boost::shared_ptr<Kernel::Units::Label> lblUnit =
        boost::dynamic_pointer_cast<Kernel::Units::Label>(
            UnitFactory::Instance().create("Label"));
    if (lblUnit) {

      if ((inputUnit->caption() == "Energy" ||
           inputUnit->caption() == "Energy transfer") &&
          inputUnit->label() == "meV") {
        lblUnit->setLabel("Time", "ns");
        df /= 2.418e2;
      } else if (inputUnit->caption() == "Time" && inputUnit->label() == "s") {
        lblUnit->setLabel("Frequency", "Hz");
      } else if (inputUnit->caption() == "Frequency" &&
                 inputUnit->label() == "Hz") {
        lblUnit->setLabel("Time", "s");
      } else if (inputUnit->caption() == "Time" &&
                 inputUnit->label() == "microsecond") {
        lblUnit->setLabel("Frequency", "MHz");
      } else if (inputUnit->caption() == "Frequency" &&
                 inputUnit->label() == "MHz") {
        lblUnit->setLabel("Time", Units::Symbol::Microsecond);
      } else if (inputUnit->caption() == "d-Spacing" &&
                 inputUnit->label() == "Angstrom") {
        lblUnit->setLabel("q", Units::Symbol::InverseAngstrom);
      } else if (inputUnit->caption() == "q" &&
                 inputUnit->label() == "Angstrom^-1") {
        lblUnit->setLabel("d-Spacing", Units::Symbol::Angstrom);
      }

      outWS->getAxis(0)->unit() = lblUnit;
    }
  }

  // centerShift == true means that the zero on the x axis is assumed to be in
  // the data centre
  // at point with index i = ySize/2. If shift == false the zero is at i = 0
  const bool centerShift = true;

  auto tAxis = new API::TextAxis(nOut);
  int iRe = 0;
  int iIm = 1;
  int iAbs = 2;
  if (addPositiveOnly) {
    iRe = 3;
    iIm = 4;
    iAbs = 5;
    tAxis->setLabel(0, "Real Positive");
    tAxis->setLabel(1, "Imag Positive");
    tAxis->setLabel(2, "Modulus Positive");
  }
  tAxis->setLabel(iRe, "Real");
  tAxis->setLabel(iIm, "Imag");
  tAxis->setLabel(iAbs, "Modulus");
  outWS->replaceAxis(1, tAxis);

  const int dys = ySize % 2;
  if (transform == "Forward") {
    /* If we translate the X-axis by -dx*ySize/2 and assume that our function is
     * periodic
     * along the X-axis with period equal to ySize, then dataY values must be
     * rearranged such that
     * dataY[i] = dataY[(ySize/2 + i + dys) % ySize]. However, we do not
     * overwrite dataY but
     * store the rearranged values in array 'data'.
     * When index 'i' runs from 0 to ySize/2, data[2*i] will store dataY[j] with
     * j running from
     * ySize/2 to ySize.  When index 'i' runs from ySize/2+1 to ySize, data[2*i]
     * will store
     * dataY[j] with j running from 0 to ySize.
     */
    for (int i = 0; i < ySize; i++) {
      int j = centerShift ? (ySize / 2 + i) % ySize : i;
      data[2 * i] =
          inWS->dataY(iReal)[j]; // even indexes filled with the real part
      data[2 * i + 1] = isComplex
                            ? inImagWS->dataY(iImag)[j]
                            : 0.; // odd indexes filled with the imaginary part
    }

    double shift =
        getProperty("Shift"); // extra phase to be applied to the transform
    shift *= 2 * M_PI;

    gsl_fft_complex_forward(data.get(), 1, ySize, wavetable, workspace);
    /* The Fourier transform overwrites array 'data'. Recall that the Fourier
     * transform is
     * periodic along the frequency axis. Thus, 'data' takes the same values
     * when index j runs
     * from ySize/2 to ySize than if index j would run from -ySize/2 to 0. Thus,
     * for negative
     * frequencies running from -ySize/2 to 0, we use the values stored in array
     * 'data'
     * for index j running from ySize/2 to ySize.
     */
    for (int i = 0; i < ySize; i++) {
      int j = (ySize / 2 + i + dys) % ySize;
      outWS->dataX(iRe)[i] =
          df * (-ySize / 2 + i); // zero frequency at i = ySize/2
      double re = data[2 * j] *
                  dx; // use j from ySize/2 to ySize for negative frequencies
      double im = data[2 * j + 1] * dx;
      // shift
      {
        double c = cos(outWS->dataX(iRe)[i] * shift);
        double s = sin(outWS->dataX(iRe)[i] * shift);
        double re1 = re * c - im * s;
        double im1 = re * s + im * c;
        re = re1;
        im = im1;
      }
      outWS->dataY(iRe)[i] = re;                       // real part
      outWS->dataY(iIm)[i] = im;                       // imaginary part
      outWS->dataY(iAbs)[i] = sqrt(re * re + im * im); // modulus
      if (addPositiveOnly) {
        outWS->dataX(0)[i] = df * i;
        if (j < ySize / 2) {
          outWS->dataY(0)[j] = re;                      // real part
          outWS->dataY(1)[j] = im;                      // imaginary part
          outWS->dataY(2)[j] = sqrt(re * re + im * im); // modulus
        } else {
          outWS->dataY(0)[j] = 0.; // real part
          outWS->dataY(1)[j] = 0.; // imaginary part
          outWS->dataY(2)[j] = 0.; // modulus
        }
      }
    }
    if (xSize == ySize + 1) {
      outWS->dataX(0)[ySize] = outWS->dataX(0)[ySize - 1] + df;
      if (addPositiveOnly)
        outWS->dataX(iRe)[ySize] = outWS->dataX(iRe)[ySize - 1] + df;
    }
  } else // Backward
  {
    for (int i = 0; i < ySize; i++) {
      int j = (ySize / 2 + i) % ySize;
      data[2 * i] = inWS->dataY(iReal)[j];
      data[2 * i + 1] = isComplex ? inImagWS->dataY(iImag)[j] : 0.;
    }
    gsl_fft_complex_inverse(data.get(), 1, ySize, wavetable, workspace);
    for (int i = 0; i < ySize; i++) {
      double x = df * i;
      if (centerShift)
        x -= df * (ySize / 2);
      outWS->dataX(0)[i] = x;
      int j = centerShift ? (ySize / 2 + i + dys) % ySize : i;
      double re = data[2 * j] / df;
      double im = data[2 * j + 1] / df;
      outWS->dataY(0)[i] = re;                      // real part
      outWS->dataY(1)[i] = im;                      // imaginary part
      outWS->dataY(2)[i] = sqrt(re * re + im * im); // modulus
    }
    if (xSize == ySize + 1)
      outWS->dataX(0)[ySize] = outWS->dataX(0)[ySize - 1] + df;
  }

  gsl_fft_complex_wavetable_free(wavetable);
  gsl_fft_complex_workspace_free(workspace);

  outWS->dataX(1) = outWS->dataX(0);
  outWS->dataX(2) = outWS->dataX(0);

  if (addPositiveOnly) {
    outWS->dataX(iIm) = outWS->dataX(iRe);
    outWS->dataX(iAbs) = outWS->dataX(iRe);
  }

  setProperty("OutputWorkspace", outWS);
}

/**
 * Perform validation of input properties:
 * - input workspace must not be empty
 * - X values must be evenly spaced (unless accepting rounding errors)
 * @returns :: map of property names to errors (empty map if no errors)
 */
std::map<std::string, std::string> FFT::validateInputs() {
  std::map<std::string, std::string> errors;

  MatrixWorkspace_const_sptr inWS = getProperty("InputWorkspace");
  const int iReal = getProperty("Real");
  const MantidVec &X = inWS->readX(iReal);

  // check that the workspace isn't empty
  if (X.size() < 2) {
    errors["InputWorkspace"] = "Input workspace must have at least two values";
  } else {
    // Check that the x values are evenly spaced
    // If accepting rounding errors, just give a warning if bins are different.
    if (areBinWidthsUneven(X)) {
      errors["InputWorkspace"] =
          "X axis must be linear (all bins have same width)";
    }
  }

  return errors;
}

/**
 * Test input X vector for spacing of values.
 * In normal use, return true if not evenly spaced (error).
 * If accepting rounding errors, threshold is more lenient, and don't return an
 * error but just warn the user.
 * @param xValues :: [input] Values to check
 * @returns :: True if unevenly spaced, False if not (or accepting errors)
 */
bool FFT::areBinWidthsUneven(const MantidVec &xValues) const {
  bool widthsUneven = false;
  const bool acceptXRoundingErrors = getProperty("AcceptXRoundingErrors");
  const double tolerance = acceptXRoundingErrors ? 0.5 : 1e-7;
  const double warnValue = 0.1;

  // Width to check against
  const double dx = [&] {
    if (acceptXRoundingErrors) {
      // use average bin width
      return (xValues[xValues.size() - 1] - xValues[0]) / (xValues.size() - 1);
    } else {
      // use first bin width
      return xValues[1] - xValues[0];
    }
  }();

  // Use cumulative errors if we are accepting rounding errors.
  // Otherwise just compare each difference in turn to the tolerance.
  auto difference = [&](size_t i) {
    if (acceptXRoundingErrors) {
      return std::abs((xValues[i] - xValues[0] - i * dx) / dx);
    } else {
      return std::abs(dx - xValues[i + 1] + xValues[i]) / dx;
    }
  };

  // Check each width against dx
  for (size_t i = 1; i < xValues.size() - 2; i++) {
    const double diff = difference(i);
    if (diff > tolerance) {
      // return an actual error
      g_log.error() << "dx=" << xValues[i + 1] - xValues[i] << ' ' << dx << ' '
                    << i << std::endl;
      widthsUneven = true;
      break;
    } else if (acceptXRoundingErrors && diff > warnValue) {
      // just warn the user
      g_log.warning() << "Bin widths differ by more than " << warnValue * 100
                      << "% of average." << std::endl;
    }
  }

  return widthsUneven;
}

} // namespace Algorithm
} // namespace Mantid
