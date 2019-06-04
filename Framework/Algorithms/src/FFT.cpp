// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FFT.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TextAxis.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/EqualBinsChecker.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/UnitLabelTypes.h"

#include <boost/shared_array.hpp>

#include <gsl/gsl_errno.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <numeric>
#include <sstream>

namespace Mantid {
namespace Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(FFT)

using namespace Kernel;
using namespace API;
using namespace DataObjects;
using namespace HistogramData;

/// Initialisation method. Declares properties to be used in algorithm.
void FFT::init() {
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
      "The name of the input workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "The name of the output workspace.");
  // if desired, provide the imaginary part in a separate workspace.
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputImagWorkspace", "",
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
  declareProperty("Shift", 0.0,
                  "Apply an extra phase equal to this quantity "
                  "times 2*pi to the transform");
  declareProperty("AutoShift", false,
                  "Automatically calculate and apply phase shift. Zero on the "
                  "X axis is assumed to be in the centre - if it is not, "
                  "setting this property will automatically correct for this.");
  declareProperty(
      "AcceptXRoundingErrors", false,
      "Continue to process the data even if X values are not evenly spaced",
      Direction::Input);

  // "Shift" should only be enabled if "AutoShift" is turned off
  setPropertySettings(
      "Shift", std::make_unique<EnabledWhenProperty>("AutoShift", IS_DEFAULT));
}

/** Executes the algorithm
 *
 *  The bins of the spectrum being transformed must have constant width
 *  (unless AcceptXRoundingErrors is set to true)
 */
void FFT::exec() {
  m_inWS = getProperty("InputWorkspace");
  m_inImagWS = getProperty("InputImagWorkspace");

  if (m_inImagWS == nullptr)
    m_inImagWS = m_inWS; // workspaces are one and the same

  const int iReal = getProperty("Real");
  const int iImag = getProperty("Imaginary");
  const bool isComplex = iImag != EMPTY_INT();

  const auto &xPoints = m_inWS->points(iReal);
  const int nPoints = static_cast<int>(xPoints.size());

  boost::shared_array<double> data(new double[2 * nPoints]);
  const std::string transform = getProperty("Transform");

  // The number of spectra in the output workspace
  int nOut = 3;
  bool addPositiveOnly = false;
  // If the input is real add 3 more spectra with positive "frequencies" only
  if (!isComplex && transform == "Forward") {
    nOut += 3;
    addPositiveOnly = true;
  }

  m_outWS = create<HistoWorkspace>(*m_inWS, nOut, Points(nPoints));

  for (int i = 0; i < nOut; ++i)
    m_outWS->getSpectrum(i).setDetectorID(static_cast<detid_t>(i + 1));

  const double dx = xPoints[1] - xPoints[0];
  double df = 1.0 / (dx * nPoints);

  // Output label
  createUnitsLabels(df);

  setupTAxis(nOut, addPositiveOnly);

  const int dys = nPoints % 2;

  m_wavetable = gsl_fft_complex_wavetable_alloc(nPoints);
  m_workspace = gsl_fft_complex_workspace_alloc(nPoints);

  // Hardcoded "centerShift == true" means that the zero on the x axis is
  // assumed to be in the centre, at point with index i = ySize/2.
  // Set to false to make zero at i = 0.
  const bool centerShift = true;

  if (transform == "Forward") {
    transformForward(data, nPoints, nPoints, dys, addPositiveOnly, centerShift,
                     isComplex, iReal, iImag, df, dx);
  } else { // Backward
    transformBackward(data, nPoints, nPoints, dys, centerShift, isComplex,
                      iReal, iImag, df);
  }

  m_outWS->setSharedX(1, m_outWS->sharedX(0));
  m_outWS->setSharedX(2, m_outWS->sharedX(0));

  if (addPositiveOnly) {
    m_outWS->setSharedX(m_iIm, m_outWS->sharedX(m_iRe));
    m_outWS->setSharedX(m_iAbs, m_outWS->sharedX(m_iRe));
  }

  gsl_fft_complex_wavetable_free(m_wavetable);
  gsl_fft_complex_workspace_free(m_workspace);

  setProperty("OutputWorkspace", m_outWS);
}

void FFT::transformForward(boost::shared_array<double> &data, const int xSize,
                           const int ySize, const int dys,
                           const bool addPositiveOnly, const bool centerShift,
                           const bool isComplex, const int iReal,
                           const int iImag, const double df, const double dx) {
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
    data[2 * i] = m_inWS->y(iReal)[j]; // even indexes filled with the real part
    data[2 * i + 1] = isComplex
                          ? m_inImagWS->y(iImag)[j]
                          : 0.; // odd indexes filled with the imaginary part
  }

  double shift = getPhaseShift(
      m_inWS->points(iReal)); // extra phase to be applied to the transform

  gsl_fft_complex_forward(data.get(), 1, ySize, m_wavetable, m_workspace);

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
    m_outWS->mutableX(m_iRe)[i] =
        df * (-ySize / 2 + i); // zero frequency at i = ySize/2
    double re = data[2 * j] *
                dx; // use j from ySize/2 to ySize for negative frequencies
    double im = data[2 * j + 1] * dx;
    // shift
    {
      double c = cos(m_outWS->x(m_iRe)[i] * shift);
      double s = sin(m_outWS->x(m_iRe)[i] * shift);
      double re1 = re * c - im * s;
      double im1 = re * s + im * c;
      re = re1;
      im = im1;
    }
    m_outWS->mutableY(m_iRe)[i] = re;                       // real part
    m_outWS->mutableY(m_iIm)[i] = im;                       // imaginary part
    m_outWS->mutableY(m_iAbs)[i] = sqrt(re * re + im * im); // modulus
    if (addPositiveOnly) {
      m_outWS->dataX(0)[i] = df * i;
      if (j < ySize / 2) {
        m_outWS->mutableY(0)[j] = re;                      // real part
        m_outWS->mutableY(1)[j] = im;                      // imaginary part
        m_outWS->mutableY(2)[j] = sqrt(re * re + im * im); // modulus
      } else {
        m_outWS->mutableY(0)[j] = 0.; // real part
        m_outWS->mutableY(1)[j] = 0.; // imaginary part
        m_outWS->mutableY(2)[j] = 0.; // modulus
      }
    }
  }
  if (xSize == ySize + 1) {
    m_outWS->mutableX(0)[ySize] = m_outWS->x(0)[ySize - 1] + df;
    if (addPositiveOnly)
      m_outWS->mutableX(m_iRe)[ySize] = m_outWS->x(m_iRe)[ySize - 1] + df;
  }
}

void FFT::transformBackward(boost::shared_array<double> &data, const int xSize,
                            const int ySize, const int dys,
                            const bool centerShift, const bool isComplex,
                            const int iReal, const int iImag, const double df) {
  for (int i = 0; i < ySize; i++) {
    int j = (ySize / 2 + i) % ySize;
    data[2 * i] = m_inWS->y(iReal)[j];
    data[2 * i + 1] = isComplex ? m_inImagWS->y(iImag)[j] : 0.;
  }

  gsl_fft_complex_inverse(data.get(), 1, ySize, m_wavetable, m_workspace);

  for (int i = 0; i < ySize; i++) {
    double x = df * i;
    if (centerShift) {
      x -= df * (ySize / 2);
    }
    m_outWS->mutableX(0)[i] = x;
    int j = centerShift ? (ySize / 2 + i + dys) % ySize : i;
    double re = data[2 * j] / df;
    double im = data[2 * j + 1] / df;
    m_outWS->mutableY(0)[i] = re;                      // real part
    m_outWS->mutableY(1)[i] = im;                      // imaginary part
    m_outWS->mutableY(2)[i] = sqrt(re * re + im * im); // modulus
  }
  if (xSize == ySize + 1)
    m_outWS->mutableX(0)[ySize] = m_outWS->x(0)[ySize - 1] + df;
}

void FFT::setupTAxis(const int nOut, const bool addPositiveOnly) {
  auto tAxis = new API::TextAxis(nOut);

  m_iRe = 0;
  m_iIm = 1;
  m_iAbs = 2;
  if (addPositiveOnly) {
    m_iRe = 3;
    m_iIm = 4;
    m_iAbs = 5;
    tAxis->setLabel(0, "Real Positive");
    tAxis->setLabel(1, "Imag Positive");
    tAxis->setLabel(2, "Modulus Positive");
  }
  tAxis->setLabel(m_iRe, "Real");
  tAxis->setLabel(m_iIm, "Imag");
  tAxis->setLabel(m_iAbs, "Modulus");
  m_outWS->replaceAxis(1, tAxis);
}

void FFT::createUnitsLabels(double &df) {
  m_outWS->getAxis(0)->unit() = UnitFactory::Instance().create("Label");

  auto inputUnit = m_inWS->getAxis(0)->unit();
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
      m_outWS->getAxis(0)->unit() = lblUnit;
    }
  }
}

/**
 * Perform validation of input properties:
 * - input workspace must not be empty
 * - X values must be evenly spaced (unless accepting rounding errors)
 * - Real and Imaginary spectra must be in range of input workspace
 * - If complex, real and imaginary workspaces must be the same size
 * @returns :: map of property names to errors (empty map if no errors)
 */
std::map<std::string, std::string> FFT::validateInputs() {
  std::map<std::string, std::string> errors;

  MatrixWorkspace_const_sptr inWS = getProperty("InputWorkspace");
  if (inWS) {
    const int iReal = getProperty("Real");
    const int iImag = getProperty("Imaginary");
    auto &X = inWS->x(iReal);

    // check that the workspace isn't empty
    if (X.size() < 2) {
      errors["InputWorkspace"] =
          "Input workspace must have at least two values";
    } else {
      // Check that the x values are evenly spaced
      // If accepting rounding errors, just give a warning if bins are
      // different.
      if (areBinWidthsUneven(inWS->binEdges(iReal))) {
        errors["InputWorkspace"] =
            "X axis must be linear (all bins have same width)";
      }
    }

    // check real, imaginary spectrum numbers and workspace sizes
    int nHist = static_cast<int>(inWS->getNumberHistograms());
    if (iReal >= nHist) {
      errors["Real"] = "Real out of range";
    }
    if (iImag != EMPTY_INT()) {
      MatrixWorkspace_const_sptr inImagWS = getProperty("InputImagWorkspace");
      if (inImagWS) {
        if (inWS->blocksize() != inImagWS->blocksize()) {
          errors["Imaginary"] = "Real and Imaginary sizes do not match";
        }
        nHist = static_cast<int>(inImagWS->getNumberHistograms());
      }
      if (iImag >= nHist) {
        errors["Imaginary"] = "Imaginary out of range";
      }
    }
  }

  return errors;
}

/**
 * Test input X vector for spacing of values.
 * In normal use, return true if not evenly spaced (error).
 * If accepting rounding errors, threshold is more lenient, and don't return an
 * error but just warn the user.
 * @param xBins :: [input] Values to check
 * @returns :: True if unevenly spaced, False if not (or accepting errors)
 */
bool FFT::areBinWidthsUneven(const HistogramData::BinEdges &xBins) const {
  const bool acceptXRoundingErrors = getProperty("AcceptXRoundingErrors");
  const double tolerance = acceptXRoundingErrors ? 0.5 : 1e-7;
  const double warnValue = acceptXRoundingErrors ? 0.1 : -1;

  // TODO should be added to HistogramData as a convenience function
  Kernel::EqualBinsChecker binChecker(xBins.rawData(), tolerance, warnValue);

  // Compatibility with previous behaviour
  if (!acceptXRoundingErrors) {
    // Compare each bin width to the first (not the average)
    binChecker.setReferenceBin(EqualBinsChecker::ReferenceBin::First);
    // Use individual errors (not cumulative)
    binChecker.setErrorType(EqualBinsChecker::ErrorType::Individual);
  }

  const std::string binError = binChecker.validate();
  return !binError.empty();
}

/**
 * Returns the phase shift to apply
 * If "AutoShift" is set, calculates this automatically as -X[N/2]
 * Otherwise, returns user-supplied "Shift" (or zero if none set)
 * @param xPoints :: [input] Reference to X points of input workspace
 * @returns :: Phase shift
 */
double FFT::getPhaseShift(const HistogramData::Points &xPoints) {
  double shift = 0.0;
  const bool autoshift = getProperty("AutoShift");
  if (autoshift) {
    const size_t mid = xPoints.size() / 2;
    shift = -xPoints[mid];
  } else {
    shift = getProperty("Shift");
  }
  shift *= 2 * M_PI;
  return shift;
}

} // namespace Algorithms
} // namespace Mantid
