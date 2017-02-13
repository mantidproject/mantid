//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Algorithms/SplineBackground.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidKernel/BoundedValidator.h"

#include <gsl/gsl_bspline.h>
#include <gsl/gsl_multifit.h>
#include <gsl/gsl_statistics.h>

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {

DECLARE_ALGORITHM(SplineBackground)

using namespace Kernel;
using namespace API;

/// Initialisation method
void SplineBackground::init() {
  declareProperty(make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "The name of the input workspace.");
  declareProperty(make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The name to use for the output workspace.");
  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty("WorkspaceIndex", 0, mustBePositive,
                  "The index of the spectrum for fitting.");
  declareProperty("NCoeff", 10, "The number of b-spline coefficients.");
}

/** Executes the algorithm
 *
 */
void SplineBackground::exec() {
  API::MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");
  const int ncoeffs = getProperty("NCoeff");
  const int spec = getProperty("WorkspaceIndex");

  // Check if the user specified a spectrum number its valid
  if (spec > static_cast<int>(inWS->getNumberHistograms()))
    throw std::out_of_range("WorkspaceIndex is out of range.");

  /* this is the data to be fitted */
  const int numBins = static_cast<int>(calculateNumBinsToProcess(inWS.get()));

  if (numBins < ncoeffs) {
    throw std::out_of_range("Too many basis functions (NCoeff)");
  }

  allocateBSplinePointers(numBins, ncoeffs);
  addWsDataToSpline(inWS.get(), spec, numBins);

  const auto &xInputPoints = inWS->x(spec);
  setupSpline(xInputPoints.front(), xInputPoints.back(), numBins, ncoeffs);

  // Wrap this in its own block so we can alias the member variable
  // to a shorter name for the duration of this call
  {
    // Have to pass a pointer so it can write chi ^ 2 even though we don't
    // use this value
    double chisq;
    // Create temporary alias to reduce the size of the call
    bSplinePointers &sp = m_splinePointers;
    // do the fit
    gsl_multifit_wlinear(sp.fittedWs, sp.binWeights, sp.yData, sp.coefficients,
                         sp.covariance, &chisq, sp.weightedLinearFitWs);
  }

  auto outWS = saveSplineOutput(inWS, spec);
  freeBSplinePointers();
  setProperty("OutputWorkspace", outWS);
}

size_t
SplineBackground::calculateNumBinsToProcess(const API::MatrixWorkspace *ws) {
  const int spec = getProperty("WorkspaceIndex");
  const auto &yInputVals = ws->y(spec);
  size_t ySize = yInputVals.size();

  // Check and subtract masked bins from the count
  bool isMasked = ws->hasMaskedBins(spec);
  if (isMasked) {
    ySize -= static_cast<int>(ws->maskedBins(spec).size());
  }
  return ySize;
}

void SplineBackground::addWsDataToSpline(const API::MatrixWorkspace *ws,
                                         const size_t specNum,
                                         int expectedNumBins) {
  const auto xInputVals = ws->x(specNum);
  const auto yInputVals = ws->y(specNum);
  const auto eInputVals = ws->e(specNum);
  const bool hasMaskedBins = ws->hasMaskedBins(specNum);

  // Mark masked bins to skip them later
  std::vector<bool> masked(yInputVals.size(), false);
  if (hasMaskedBins) {
    const auto maskedBinsMap = ws->maskedBins(specNum);
    for (const auto &maskedBin : maskedBinsMap) {
      masked[maskedBin.first] = true;
    }
  }

  int numUnmaskedBins = 0;
  for (size_t i = 0; i < yInputVals.size(); ++i) {
    if (hasMaskedBins && masked[i])
      continue;
    gsl_vector_set(m_splinePointers.xData, numUnmaskedBins, xInputVals[i]);
    gsl_vector_set(m_splinePointers.yData, numUnmaskedBins, yInputVals[i]);
    gsl_vector_set(m_splinePointers.binWeights, numUnmaskedBins,
                   eInputVals[i] > 0. ? 1. / (eInputVals[i] * eInputVals[i])
                                      : 0.);

    ++numUnmaskedBins;
  }

  if (expectedNumBins != numUnmaskedBins) {
    freeBSplinePointers();
    throw std::runtime_error(
        "Assertion failed: number of unmasked bins found was"
        " not equal to the number that was calculated");
  }
}

void SplineBackground::allocateBSplinePointers(int numBins, int ncoeffs) {
  const int k = 4; // order of the spline + 1 (cubic)
  const int nbreak = ncoeffs - (k - 2);

  if (nbreak <= 0)
    throw std::out_of_range("Too low NCoeff");

  // Create an alias to tidy it up
  bSplinePointers &sp = m_splinePointers;
  // allocate a cubic bspline workspace (k = 4)
  sp.splineToProcess = gsl_bspline_alloc(k, nbreak);
  sp.inputSplineWs = gsl_vector_alloc(ncoeffs);

  sp.xData = gsl_vector_alloc(numBins);
  sp.yData = gsl_vector_alloc(numBins);
  sp.fittedWs = gsl_matrix_alloc(numBins, ncoeffs);
  sp.coefficients = gsl_vector_alloc(ncoeffs);
  sp.binWeights = gsl_vector_alloc(numBins);
  sp.covariance = gsl_matrix_alloc(ncoeffs, ncoeffs);
  sp.weightedLinearFitWs = gsl_multifit_linear_alloc(numBins, ncoeffs);
}

void SplineBackground::freeBSplinePointers() {
  // Create an alias to tidy it up
  bSplinePointers &sp = m_splinePointers;

  gsl_bspline_free(sp.splineToProcess);
  sp.splineToProcess = nullptr;
  gsl_vector_free(sp.inputSplineWs);
  sp.inputSplineWs = nullptr;
  gsl_vector_free(sp.xData);
  sp.xData = nullptr;
  gsl_vector_free(sp.yData);
  sp.yData = nullptr;
  gsl_matrix_free(sp.fittedWs);
  sp.fittedWs = nullptr;
  gsl_vector_free(sp.coefficients);
  sp.coefficients = nullptr;
  gsl_vector_free(sp.binWeights);
  sp.binWeights = nullptr;
  gsl_matrix_free(sp.covariance);
  sp.covariance = nullptr;
  gsl_multifit_linear_free(sp.weightedLinearFitWs);
  sp.weightedLinearFitWs = nullptr;
}

MatrixWorkspace_sptr
SplineBackground::saveSplineOutput(const API::MatrixWorkspace_sptr ws,
                                   const size_t spec) {
  const auto &xInputVals = ws->x(spec);
  const auto &yInputVals = ws->y(spec);
  /* output the smoothed curve */
  API::MatrixWorkspace_sptr outWS = WorkspaceFactory::Instance().create(
      ws, 1, xInputVals.size(), yInputVals.size());

  outWS->getSpectrum(0).setSpectrumNo(ws->getSpectrum(spec).getSpectrumNo());

  double yi, yerr;
  auto &yVals = outWS->mutableY(0);
  auto &eVals = outWS->mutableE(0);

  for (size_t i = 0; i < yInputVals.size(); i++) {
    double xi = xInputVals[i];
    gsl_bspline_eval(xi, m_splinePointers.inputSplineWs,
                     m_splinePointers.splineToProcess);
    gsl_multifit_linear_est(m_splinePointers.inputSplineWs,
                            m_splinePointers.coefficients,
                            m_splinePointers.covariance, &yi, &yerr);
    yVals[i] = yi;
    eVals[i] = yerr;
  }
  outWS->setSharedX(0, ws->sharedX(0));
  return outWS;
}

void SplineBackground::setupSpline(double xMin, double xMax, int numBins,
                                   int ncoeff) {
  /* use uniform breakpoints */
  gsl_bspline_knots_uniform(xMin, xMax, m_splinePointers.splineToProcess);

  /* construct the fit matrix X */
  for (int i = 0; i < numBins; ++i) {
    double xi = gsl_vector_get(m_splinePointers.xData, i);

    /* compute B_j(xi) for all j */
    gsl_bspline_eval(xi, m_splinePointers.inputSplineWs,
                     m_splinePointers.splineToProcess);

    /* fill in row i of X */
    for (int j = 0; j < ncoeff; ++j) {
      double Bj = gsl_vector_get(m_splinePointers.inputSplineWs, j);
      gsl_matrix_set(m_splinePointers.fittedWs, i, j, Bj);
    }
  }
}

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
