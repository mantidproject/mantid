#ifndef FIT_GSL_H
#define FIT_GSL_H

#include <gsl/gsl_vector.h>

//! Structure for fitting data
struct FitData {
  size_t n;// number of points to be fitted (size of X, Y and sigma arrays)
  size_t p;// number of fit parameters
  double * X;// the data to be fitted (abscissae) 
  double * Y; // the data to be fitted (ordinates)
  double * sigma; // the weighting data
  const char *function; // fit model (used only by the NonLinearFit class)
  const char *names; // names of the fit parameters separated by "," (used only by the NonLinearFit class)
};

int expd3_fdf (const gsl_vector * x, void *params, gsl_vector * f, gsl_matrix * J);
int expd3_df (const gsl_vector * x, void *params, gsl_matrix * J);
int expd3_f (const gsl_vector * x, void *params, gsl_vector * f);
double expd3_d (const gsl_vector * x, void *params);

int expd2_fdf (const gsl_vector * x, void *params, gsl_vector * f, gsl_matrix * J);
int expd2_df (const gsl_vector * x, void *params, gsl_matrix * J);
int expd2_f (const gsl_vector * x, void *params, gsl_vector * f);
double expd2_d (const gsl_vector * x, void *params);

int exp_fdf (const gsl_vector * x, void *params, gsl_vector * f, gsl_matrix * J);
int exp_df (const gsl_vector * x, void *params, gsl_matrix * J);
int exp_f (const gsl_vector * x, void *params, gsl_vector * f);
double exp_d (const gsl_vector * x, void *params);

int boltzmann_fdf (const gsl_vector * x, void *params, gsl_vector * f, gsl_matrix * J);
int boltzmann_df (const gsl_vector * x, void *params, gsl_matrix * J);
int boltzmann_f (const gsl_vector * x, void *params, gsl_vector * f);
double boltzmann_d (const gsl_vector * x, void *params);

int logistic_fdf (const gsl_vector * x, void *params, gsl_vector * f, gsl_matrix * J);
int logistic_df (const gsl_vector * x, void *params, gsl_matrix * J);
int logistic_f (const gsl_vector * x, void *params, gsl_vector * f);
double logistic_d (const gsl_vector * x, void *params);

int gauss_fdf (const gsl_vector * x, void *params, gsl_vector * f, gsl_matrix * J);
int gauss_df (const gsl_vector * x, void *params, gsl_matrix * J);
int gauss_f (const gsl_vector * x, void *params,gsl_vector * f);
double gauss_d (const gsl_vector * x, void *params);

int gauss_multi_peak_f (const gsl_vector * x, void *params, gsl_vector * f);
double gauss_multi_peak_d (const gsl_vector * x, void *params);
int gauss_multi_peak_df (const gsl_vector * x, void *params, gsl_matrix * J);
int gauss_multi_peak_fdf (const gsl_vector * x, void *params, gsl_vector * f, gsl_matrix * J);

int lorentz_multi_peak_f (const gsl_vector * x, void *params, gsl_vector * f);
double lorentz_multi_peak_d (const gsl_vector * x, void *params);
int lorentz_multi_peak_df (const gsl_vector * x, void *params, gsl_matrix * J);
int lorentz_multi_peak_fdf (const gsl_vector * x, void *params, gsl_vector * f, gsl_matrix * J);

int user_f(const gsl_vector * x, void *params, gsl_vector * f);
double user_d(const gsl_vector * x, void *params);
int user_df(const gsl_vector * x, void *params,gsl_matrix * J);
int user_fdf(const gsl_vector * x, void *params,gsl_vector * f, gsl_matrix * J);

#endif
