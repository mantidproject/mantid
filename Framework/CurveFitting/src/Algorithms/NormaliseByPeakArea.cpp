// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Algorithms/NormaliseByPeakArea.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidHistogramData/HistogramE.h"
#include "MantidHistogramData/HistogramY.h"

#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"

#include <boost/make_shared.hpp>

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {

/// Starting value of peak position in y-space for fit
double PEAK_POS_GUESS = -0.1;
/// Starting value of width in y-space for fit
double PEAK_WIDTH_GUESS = 4.0;
/// Bin width for rebinning workspace converted from TOF
double SUMMEDY_BIN_WIDTH = 0.5;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(NormaliseByPeakArea)

using namespace API;
using namespace Kernel;

//----------------------------------------------------------------------------------------------
/** Constructor
 */
NormaliseByPeakArea::NormaliseByPeakArea()
    : API::Algorithm(), m_inputWS(), m_mass(0.0), m_sumResults(true),
      m_normalisedWS(), m_yspaceWS(), m_fittedWS(), m_symmetrisedWS(),
      m_progress() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string NormaliseByPeakArea::name() const {
  return "NormaliseByPeakArea";
}

/// Algorithm's version for identification. @see Algorithm::version
int NormaliseByPeakArea::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string NormaliseByPeakArea::category() const {
  return "CorrectionFunctions\\NormalisationCorrections";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void NormaliseByPeakArea::init() {
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<HistogramValidator>(false); // point data
  wsValidator->add<InstrumentValidator>();
  wsValidator->add<WorkspaceUnitValidator>("TOF");
  declareProperty(std::make_unique<WorkspaceProperty<>>(
                      "InputWorkspace", "", Direction::Input, wsValidator),
                  "An input workspace.");

  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  mustBePositive->setLowerExclusive(true); // strictly greater than 0.0
  declareProperty("Mass", -1.0, mustBePositive,
                  "The mass, in AMU, defining the recoil peak to fit");
  declareProperty(
      "Sum", true,
      "If true all spectra on the Y-space, fitted & symmetrised workspaces "
      "are summed in quadrature to produce the final result");

  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                        Direction::Output),
                  "Input workspace normalised by the fitted peak area");
  declareProperty(std::make_unique<WorkspaceProperty<>>("YSpaceDataWorkspace",
                                                        "", Direction::Output),
                  "Input workspace converted to units of Y-space");
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("FittedWorkspace", "",
                                            Direction::Output),
      "Output from fit of the single mass peakin y-space. The output units are "
      "in momentum (A^-1)");
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("SymmetrisedWorkspace", "",
                                            Direction::Output),
      "The input data symmetrised about Y=0.  The output units are in momentum "
      "(A^-1)");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void NormaliseByPeakArea::exec() {
  retrieveInputs();
  const auto yspaceIn = convertInputToY();
  createOutputWorkspaces(yspaceIn);

  const auto nhist = static_cast<int64_t>(yspaceIn->getNumberHistograms());
  const auto nreports =
      static_cast<int64_t>(yspaceIn->getNumberHistograms() +
                           2 * m_symmetrisedWS->getNumberHistograms() *
                               m_symmetrisedWS->blocksize());
  m_progress = new API::Progress(this, 0.10, 1.0, nreports);

  for (int64_t i = 0; i < nhist; ++i) {
    m_normalisedWS->setSharedX(i, m_inputWS->sharedX(i)); // TOF
    if (!m_sumResults) // avoid setting multiple times if we are summing
    {
      m_yspaceWS->setSharedX(i, yspaceIn->sharedX(i));      // momentum
      m_fittedWS->setSharedX(i, yspaceIn->sharedX(i));      // momentum
      m_symmetrisedWS->setSharedX(i, yspaceIn->sharedX(i)); // momentum
    }

    double peakArea = fitToMassPeak(yspaceIn, static_cast<size_t>(i));
    normaliseTOFData(peakArea, i);
    saveToOutput(m_yspaceWS, yspaceIn->sharedY(i), yspaceIn->sharedE(i), i);

    m_progress->report();
  }
  // This has to be done after the summation of the spectra
  symmetriseYSpace();

  setProperty("OutputWorkspace", m_normalisedWS);
  setProperty("YSpaceDataWorkspace", m_yspaceWS);
  setProperty("FittedWorkspace", m_fittedWS);
  setProperty("SymmetrisedWorkspace", m_symmetrisedWS);
}

/**
 * Caches input details for the peak information
 */
void NormaliseByPeakArea::retrieveInputs() {
  m_inputWS = getProperty("InputWorkspace");
  m_mass = getProperty("Mass");
  m_sumResults = getProperty("Sum");
}

/**
 * Creates & cache output workspaces.
 * @param yspaceIn Workspace containing TOF input values converted to Y-space
 */
void NormaliseByPeakArea::createOutputWorkspaces(
    const API::MatrixWorkspace_sptr &yspaceIn) {
  m_normalisedWS =
      WorkspaceFactory::Instance().create(m_inputWS); // TOF data is not resized

  const size_t nhist = m_sumResults ? 1 : yspaceIn->getNumberHistograms();

  m_yspaceWS = WorkspaceFactory::Instance().create(yspaceIn, nhist);
  m_fittedWS = WorkspaceFactory::Instance().create(yspaceIn, nhist);
  m_symmetrisedWS = WorkspaceFactory::Instance().create(yspaceIn, nhist);
  if (m_sumResults) {
    // Copy over xvalues & assign "high" initial error values to simplify
    // symmetrisation calculation
    constexpr double high(1e6);

    m_yspaceWS->setSharedX(0, yspaceIn->sharedX(0));
    m_fittedWS->setSharedX(0, yspaceIn->sharedX(0));
    m_symmetrisedWS->setSharedX(0, yspaceIn->sharedX(0));
    m_yspaceWS->mutableE(0) = high;
    m_fittedWS->setSharedE(0, m_yspaceWS->sharedE(0));
    m_symmetrisedWS->setSharedE(0, m_yspaceWS->sharedE(0));
  }

  setUnitsToMomentum(m_yspaceWS);
  setUnitsToMomentum(m_fittedWS);
  setUnitsToMomentum(m_symmetrisedWS);
}

/**
 * @param workspace Workspace whose units should be altered
 */
void NormaliseByPeakArea::setUnitsToMomentum(
    const API::MatrixWorkspace_sptr &workspace) {
  // Units
  auto xLabel = boost::make_shared<Units::Label>("Momentum", "A^-1");
  workspace->getAxis(0)->unit() = xLabel;
  workspace->setYUnit("");
  workspace->setYUnitLabel("");
}

/*
 * Returns a workspace converted to Y-space coordinates. @see ConvertToYSpace.
 * If summing is requested
 * then the output is rebinned to a common grid to allow summation onto a common
 * grid. The rebin min/max
 * is found from the converted workspace
 */
MatrixWorkspace_sptr NormaliseByPeakArea::convertInputToY() {
  auto alg = createChildAlgorithm("ConvertToYSpace", 0.0, 0.05, false);
  alg->setProperty("InputWorkspace", m_inputWS);
  alg->setProperty("Mass", m_mass);
  alg->execute();
  MatrixWorkspace_sptr tofInY = alg->getProperty("OutputWorkspace");
  if (!m_sumResults)
    return tofInY;

  // Rebin to common grid
  double xmin(0.0), xmax(0.0);
  tofInY->getXMinMax(xmin, xmax);
  std::vector<double> params(3);
  params[0] = xmin;
  params[1] = SUMMEDY_BIN_WIDTH;
  params[2] = xmax;

  alg = createChildAlgorithm("Rebin", 0.05, 0.1, false);
  alg->setProperty("InputWorkspace", tofInY);
  alg->setProperty("Params", params);
  alg->execute();
  return alg->getProperty("OutputWorkspace");
}

/**
 * Runs fit using the ComptonPeakProfile function on the given spectrum of the
 * input workspace to determine
 * the peak area for the input mass
 * @param yspace A workspace in units of Y
 * @param index Index of the spectrum to fit
 * @return The value of the peak area
 */
double NormaliseByPeakArea::fitToMassPeak(const MatrixWorkspace_sptr &yspace,
                                          const size_t index) {
  auto alg = createChildAlgorithm("Fit");
  auto func = FunctionFactory::Instance().createFunction("ComptonPeakProfile");
  func->setAttributeValue("Mass", m_mass);
  func->setAttributeValue("WorkspaceIndex", static_cast<int>(index));

  // starting guesses based on Hydrogen spectrum
  func->setParameter("Position", PEAK_POS_GUESS);
  func->setParameter("SigmaGauss", PEAK_WIDTH_GUESS);

  // Guess at intensity
  const size_t npts = yspace->blocksize();
  const auto &yVals = yspace->y(index);
  const auto &xVals = yspace->x(index);
  double areaGuess(0.0);
  for (size_t j = 1; j < npts; ++j) {
    areaGuess += yVals[j - 1] * (xVals[j] - xVals[j - 1]);
  }
  func->setParameter("Intensity", areaGuess);
  if (g_log.is(Logger::Priority::PRIO_DEBUG)) {
    g_log.debug() << "Starting values for peak fit on spectrum "
                  << yspace->getSpectrum(index).getSpectrumNo() << ":\n"
                  << "area=" << areaGuess << "\n"
                  << "width=" << PEAK_WIDTH_GUESS << "\n"
                  << "position=" << PEAK_POS_GUESS << "\n";
  }
  alg->setProperty("Function", func);
  alg->setProperty("InputWorkspace",
                   boost::static_pointer_cast<Workspace>(yspace));
  alg->setProperty("WorkspaceIndex", static_cast<int>(index));
  alg->setProperty("CreateOutput", true);
  alg->execute();

  MatrixWorkspace_sptr fitOutputWS = alg->getProperty("OutputWorkspace");
  saveToOutput(m_fittedWS, fitOutputWS->sharedY(1), yspace->sharedE(index),
               index);

  double area = func->getParameter("Intensity");
  if (g_log.is(Logger::Priority::PRIO_INFORMATION)) {
    g_log.information() << "Calculated peak area for spectrum "
                        << yspace->getSpectrum(index).getSpectrumNo() << ": "
                        << area << "\n";
  }
  return area;
}

/**
 * Divides the input Y & E data by the given factor
 * @param area Value to use as normalisation factor
 * @param index Index on input spectrum to normalise
 */
void NormaliseByPeakArea::normaliseTOFData(const double area,
                                           const size_t index) {
  m_normalisedWS->mutableY(index) = m_inputWS->y(index) / area;
  m_normalisedWS->mutableE(index) = m_inputWS->e(index) / area;
}

/**
 * @param accumWS Workspace used to accumulate the final data
 * @param yValues Input signal values for y-space
 * @param eValues Input errors values for y-space
 * @param index Index of the workspace. Only used when not summing.
 */
void NormaliseByPeakArea::saveToOutput(
    const API::MatrixWorkspace_sptr &accumWS,
    const Kernel::cow_ptr<HistogramData::HistogramY> &yValues,
    const Kernel::cow_ptr<HistogramData::HistogramE> &eValues,
    const size_t index) {
  assert(yValues->rawData().size() == eValues->rawData().size());

  if (m_sumResults) {
    const size_t npts(accumWS->blocksize());
    auto &accumY = accumWS->mutableY(0);
    auto &accumE = accumWS->mutableE(0);
    const auto &yValuesRaw = yValues->rawData();
    const auto &eValuesRaw = eValues->rawData();

    for (size_t j = 0; j < npts; ++j) {
      double accumYj = accumWS->y(0)[j];
      double accumEj = accumWS->e(0)[j];
      double rhsYj = yValuesRaw[j];
      double rhsEj = eValuesRaw[j];
      if (accumEj < 1e-12 || rhsEj < 1e-12)
        continue;
      double err = 1.0 / (accumEj * accumEj) + 1.0 / (rhsEj * rhsEj);
      accumY[j] = accumYj / (accumEj * accumEj) + rhsYj / (rhsEj * rhsEj);
      accumY[j] /= err;
      accumE[j] = 1.0 / sqrt(err);
    }
  } else {
    accumWS->setSharedY(index, yValues);
    accumWS->setSharedE(index, eValues);
  }
}

/**
 * Symmetrises the yspace data about the origin
 */
void NormaliseByPeakArea::symmetriseYSpace() {
  // A window is defined the around the Y value of each data point & every other
  // point is
  // then checked to see if it falls with in the absolute value +/- window
  // width.
  // If it does then the signal is added using the error as a weight, i.e
  //
  //    yout(j) = yout(j)/(eout(j)^2) + y(j)/(e(j)^2)

  // Symmetrise input data in Y-space
  const double dy = 0.1;
  const size_t npts(m_yspaceWS->blocksize());
  const auto nhist =
      static_cast<int64_t>(m_symmetrisedWS->getNumberHistograms());

  PARALLEL_FOR_IF(Kernel::threadSafe(*m_symmetrisedWS))
  for (int64_t i = 0; i < nhist; ++i) {
    const auto &xsym = m_symmetrisedWS->x(i);
    auto &ySymOut = m_symmetrisedWS->mutableY(i);
    auto &eSymOut = m_symmetrisedWS->mutableE(i);
    const auto &yyspace = m_yspaceWS->y(i);
    const auto &eyspace = m_yspaceWS->e(i);

    for (size_t j = 0; j < npts; ++j) {
      const double ein = eyspace[j];
      const double absXj = fabs(xsym[j]);

      double yout(0.0), eout(1e8);
      for (size_t k = 0; k < npts; ++k) {
        const double yk = yyspace[k];
        const double ek = eyspace[k];
        const double absXk = fabs(xsym[k]);
        if (absXj >= (absXk - dy) && absXj <= (absXk + dy) && ein != 0.0) {

          if (ein > 1e-12) {
            double invE2 = 1 / (ek * ek);
            yout /= eout * eout;
            yout += yk * invE2;
            double wt = (1 / (eout * eout)) + invE2;
            yout /= wt;
            eout = sqrt(1 / wt);
          } else {
            yout = 1e-12;
            eout = 1e-12;
          }
        }
      }
      ySymOut[j] = yout;
      eSymOut[j] = eout;
      m_progress->report();
    }
  }
}

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
