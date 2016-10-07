//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Algorithms/SplineBackground.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"

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
  int spec = getProperty("WorkspaceIndex");

  if (spec > static_cast<int>(inWS->getNumberHistograms()))
    throw std::out_of_range("WorkspaceIndex is out of range.");

  const auto &yInputVals = inWS->y(spec);
  const bool isHistogram = inWS->isHistogramData();

  const int ncoeffs = getProperty("NCoeff");
  const int k = 4; // order of the spline + 1 (cubic)
  const int nbreak = ncoeffs - (k - 2);

  if (nbreak <= 0)
    throw std::out_of_range("Too low NCoeff");

  gsl_bspline_workspace *bw;
  gsl_vector *B;

  gsl_vector *c, *w, *x, *y;
  gsl_matrix *Z, *cov;
  gsl_multifit_linear_workspace *mw;
  double chisq;

  int n = static_cast<int>(yInputVals.size());
  bool isMasked = inWS->hasMaskedBins(spec);
  std::vector<int> masked(yInputVals.size());
  if (isMasked) {
    auto maskedBins = inWS->maskedBins(spec);
    for (auto &maskedBin : maskedBins)
      masked[maskedBin.first] = 1;
    n -= static_cast<int>(inWS->maskedBins(spec).size());
  }

  if (n < ncoeffs) {
    g_log.error("Too many basis functions (NCoeff)");
    throw std::out_of_range("Too many basis functions (NCoeff)");
  }

  /* allocate a cubic bspline workspace (k = 4) */
  bw = gsl_bspline_alloc(k, nbreak);
  B = gsl_vector_alloc(ncoeffs);

  x = gsl_vector_alloc(n);
  y = gsl_vector_alloc(n);
  Z = gsl_matrix_alloc(n, ncoeffs);
  c = gsl_vector_alloc(ncoeffs);
  w = gsl_vector_alloc(n);
  cov = gsl_matrix_alloc(ncoeffs, ncoeffs);
  mw = gsl_multifit_linear_alloc(n, ncoeffs);

  /* this is the data to be fitted */
  const auto &eInputVals = inWS->e(spec);
  const auto &xInputPoints = inWS->points(spec);
  int j = 0;
  for (MantidVec::size_type i = 0; i < yInputVals.size(); ++i) {
    if (isMasked && masked[i])
      continue;
    gsl_vector_set(x, j, xInputPoints[i]);
    gsl_vector_set(y, j, yInputVals[i]);
    gsl_vector_set(
        w, j, eInputVals[i] > 0. ? 1. / (eInputVals[i] * eInputVals[i]) : 0.);

    ++j;
  }

  if (n != j) {
    gsl_bspline_free(bw);
    gsl_vector_free(B);
    gsl_vector_free(x);
    gsl_vector_free(y);
    gsl_matrix_free(Z);
    gsl_vector_free(c);
    gsl_vector_free(w);
    gsl_matrix_free(cov);
    gsl_multifit_linear_free(mw);

    throw std::runtime_error("Assertion failed: n != j");
  }

  const auto &xInputVals = inWS->x(spec);

  double xStart = xInputVals.front();
  double xEnd = xInputVals.back();

  /* use uniform breakpoints */
  gsl_bspline_knots_uniform(xStart, xEnd, bw);

  /* construct the fit matrix X */
  for (int i = 0; i < n; ++i) {
    double xi = gsl_vector_get(x, i);

    /* compute B_j(xi) for all j */
    gsl_bspline_eval(xi, B, bw);

    /* fill in row i of X */
    for (j = 0; j < ncoeffs; ++j) {
      double Bj = gsl_vector_get(B, j);
      gsl_matrix_set(Z, i, j, Bj);
    }
  }

  /* do the fit */
  gsl_multifit_wlinear(Z, w, y, c, cov, &chisq, mw);

  /* output the smoothed curve */
  API::MatrixWorkspace_sptr outWS = WorkspaceFactory::Instance().create(
      inWS, 1, xInputVals.size(), yInputVals.size());
  {
    outWS->getSpectrum(0)
        .setSpectrumNo(inWS->getSpectrum(spec).getSpectrumNo());
    double yi, yerr;

    auto &yVals = outWS->mutableY(0);
    auto &eVals = outWS->mutableE(0);

    for (MantidVec::size_type i = 0; i < yInputVals.size(); i++) {
      double xi = xInputVals[i];
      gsl_bspline_eval(xi, B, bw);
      gsl_multifit_linear_est(B, c, cov, &yi, &yerr);
      yVals[i] = yi;
      eVals[i] = yerr;
    }
    outWS->setSharedX(0, inWS->sharedX(0));
  }

  gsl_bspline_free(bw);
  gsl_vector_free(B);
  gsl_vector_free(x);
  gsl_vector_free(y);
  gsl_matrix_free(Z);
  gsl_vector_free(c);
  gsl_vector_free(w);
  gsl_matrix_free(cov);
  gsl_multifit_linear_free(mw);

  setProperty("OutputWorkspace", outWS);
}

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
