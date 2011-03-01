//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/SplineBackground.h"
#include <gsl/gsl_bspline.h>
#include <gsl/gsl_multifit.h>
#include <gsl/gsl_statistics.h>

namespace Mantid
{
namespace CurveFitting
{

DECLARE_ALGORITHM(SplineBackground)

using namespace Kernel;
using namespace API;

/// Initialisation method
void SplineBackground::init()
{
  //this->setWikiSummary("Fit spectra background using b-splines.");
  //this->setOptionalMessage("Fit spectra background using b-splines.");

  declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace",
    "",Direction::Input), "The name of the input workspace.");
  declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace",
    "",Direction::Output), "The name of the output workspace.");
  BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
  mustBePositive->setLower(0);
  declareProperty("WorkspaceIndex",0,mustBePositive,"Spectrum number to use.");
  declareProperty("NCoeff",10,"The number of spline coefficients.");
}

/** Executes the algorithm
 *
 */
void SplineBackground::exec()
{

  API::MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");
  int spec = getProperty("WorkspaceIndex");

  if (spec > inWS->getNumberHistograms())
    throw std::out_of_range("WorkspaceIndex is out of range.");

  const MantidVec& X = inWS->readX(spec);
  const MantidVec& Y = inWS->readY(spec);
  const MantidVec& E = inWS->readE(spec);
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

  int n = Y.size();
  bool isMasked = inWS->hasMaskedBins(spec);
  std::vector<int> masked(Y.size());
  if (isMasked)
  {
    for(API::MatrixWorkspace::MaskList::const_iterator it=inWS->maskedBins(spec).begin();it!=inWS->maskedBins(spec).end();it++)
      masked[it->first] = 1;
    n -= inWS->maskedBins(spec).size();
  }

  if (n < ncoeffs)
  {
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
  int j = 0;
  for (MantidVec::size_type i = 0; i < Y.size(); ++i)
  {
    if (isMasked && masked[i]) continue;
    gsl_vector_set(x, j, (isHistogram ? (0.5*(X[i]+X[i+1])) : X[i])); // Middle of the bins, if a histogram
    gsl_vector_set(y, j, Y[i]);
    gsl_vector_set(w, j, E[i]>0.?1./(E[i]*E[i]):0.);

    ++j;
  }

  if (n != j)
  {
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

  double xStart = X.front();
  double xEnd =   X.back();

  /* use uniform breakpoints */
  gsl_bspline_knots_uniform(xStart, xEnd, bw);

  /* construct the fit matrix X */
  for (int i = 0; i < n; ++i)
  {
    double xi=gsl_vector_get(x, i);

    /* compute B_j(xi) for all j */
    gsl_bspline_eval(xi, B, bw);

    /* fill in row i of X */
    for (j = 0; j < ncoeffs; ++j)
    {
      double Bj = gsl_vector_get(B, j);
      gsl_matrix_set(Z, i, j, Bj);
    }
  }

  /* do the fit */
  gsl_multifit_wlinear(Z, w, y, c, cov, &chisq, mw);

  /* output the smoothed curve */
  API::MatrixWorkspace_sptr outWS = WorkspaceFactory::Instance().create(inWS,1,X.size(),Y.size());
  {
    outWS->getAxis(1)->spectraNo(0) = inWS->getAxis(1)->spectraNo(spec);
    double xi, yi, yerr;
    for (MantidVec::size_type i=0;i<Y.size();i++)
    {
      xi = X[i];
      gsl_bspline_eval(xi, B, bw);
      gsl_multifit_linear_est(B, c, cov, &yi, &yerr);
      outWS->dataY(0)[i] = yi;
      outWS->dataE(0)[i] = yerr;
    }
    outWS->dataX(0) = X;
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

  setProperty("OutputWorkspace",outWS);

}

} // namespace Algorithm
} // namespace Mantid
