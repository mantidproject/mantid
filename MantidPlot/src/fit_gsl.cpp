// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "fit_gsl.h"
#include "MyParser.h"
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_math.h>
#include <qmessagebox.h>
int expd3_f(const gsl_vector *x, void *params, gsl_vector *f) {
  size_t n = ((struct FitData *)params)->n;
  double *X = ((struct FitData *)params)->X;
  double *Y = ((struct FitData *)params)->Y;
  double *sigma = ((struct FitData *)params)->sigma;
  double A1 = gsl_vector_get(x, 0);
  double t1 = gsl_vector_get(x, 1);
  double A2 = gsl_vector_get(x, 2);
  double t2 = gsl_vector_get(x, 3);
  double A3 = gsl_vector_get(x, 4);
  double t3 = gsl_vector_get(x, 5);
  double y0 = gsl_vector_get(x, 6);
  size_t i;
  for (i = 0; i < n; i++) {
    double Yi =
        A1 * exp(-X[i] * t1) + A2 * exp(-X[i] * t2) + A3 * exp(-X[i] * t3) + y0;
    gsl_vector_set(f, i, (Yi - Y[i]) / sigma[i]);
  }
  return GSL_SUCCESS;
}
double expd3_d(const gsl_vector *x, void *params) {
  size_t n = ((struct FitData *)params)->n;
  double *X = ((struct FitData *)params)->X;
  double *Y = ((struct FitData *)params)->Y;
  double *sigma = ((struct FitData *)params)->sigma;
  double A1 = gsl_vector_get(x, 0);
  double t1 = gsl_vector_get(x, 1);
  double A2 = gsl_vector_get(x, 2);
  double t2 = gsl_vector_get(x, 3);
  double A3 = gsl_vector_get(x, 4);
  double t3 = gsl_vector_get(x, 5);
  double y0 = gsl_vector_get(x, 6);
  size_t i;
  double val = 0;
  for (i = 0; i < n; i++) {
    double dYi = ((A1 * exp(-X[i] * t1) + A2 * exp(-X[i] * t2) +
                   A3 * exp(-X[i] * t3) + y0) -
                  Y[i]) /
                 sigma[i];
    val += dYi * dYi;
  }
  return val;
}
int expd3_df(const gsl_vector *x, void *params, gsl_matrix *J) {
  size_t n = ((struct FitData *)params)->n;
  double *X = ((struct FitData *)params)->X;
  double *sigma = ((struct FitData *)params)->sigma;
  double A1 = gsl_vector_get(x, 0);
  double l1 = gsl_vector_get(x, 1);
  double A2 = gsl_vector_get(x, 2);
  double l2 = gsl_vector_get(x, 3);
  double A3 = gsl_vector_get(x, 4);
  double l3 = gsl_vector_get(x, 5);
  size_t i;
  for (i = 0; i < n; i++) {
    /* Jacobian matrix J(i,j) = dfi / dxj, */
    /* where fi = (Yi - yi)/sigma[i],      */
    /*       Yi = A1 * exp(-xi*l1) + A2 * exp(-xi*l2) +y0  */
    /* and the xj are the parameters (A1,l1,A2,l2,y0) */
    double t = X[i];
    double s = sigma[i];
    double e1 = exp(-t * l1) / s;
    double e2 = exp(-t * l2) / s;
    double e3 = exp(-t * l3) / s;
    gsl_matrix_set(J, i, 0, e1);
    gsl_matrix_set(J, i, 1, -t * A1 * e1);
    gsl_matrix_set(J, i, 2, e2);
    gsl_matrix_set(J, i, 3, -t * A2 * e2);
    gsl_matrix_set(J, i, 4, e3);
    gsl_matrix_set(J, i, 5, -t * A3 * e3);
    gsl_matrix_set(J, i, 6, 1 / s);
  }
  return GSL_SUCCESS;
}
int expd3_fdf(const gsl_vector *x, void *params, gsl_vector *f, gsl_matrix *J) {
  expd3_f(x, params, f);
  expd3_df(x, params, J);
  return GSL_SUCCESS;
}
int expd2_f(const gsl_vector *x, void *params, gsl_vector *f) {
  size_t n = ((struct FitData *)params)->n;
  double *X = ((struct FitData *)params)->X;
  double *Y = ((struct FitData *)params)->Y;
  double *sigma = ((struct FitData *)params)->sigma;
  double A1 = gsl_vector_get(x, 0);
  double t1 = gsl_vector_get(x, 1);
  double A2 = gsl_vector_get(x, 2);
  double t2 = gsl_vector_get(x, 3);
  double y0 = gsl_vector_get(x, 4);
  size_t i;
  for (i = 0; i < n; i++) {
    double Yi = A1 * exp(-X[i] * t1) + A2 * exp(-X[i] * t2) + y0;
    gsl_vector_set(f, i, (Yi - Y[i]) / sigma[i]);
  }
  return GSL_SUCCESS;
}
double expd2_d(const gsl_vector *x, void *params) {
  size_t n = ((struct FitData *)params)->n;
  double *X = ((struct FitData *)params)->X;
  double *Y = ((struct FitData *)params)->Y;
  double *sigma = ((struct FitData *)params)->sigma;
  double A1 = gsl_vector_get(x, 0);
  double t1 = gsl_vector_get(x, 1);
  double A2 = gsl_vector_get(x, 2);
  double t2 = gsl_vector_get(x, 3);
  double y0 = gsl_vector_get(x, 4);
  size_t i;
  double val = 0;
  for (i = 0; i < n; i++) {
    double dYi =
        ((A1 * exp(-X[i] * t1) + A2 * exp(-X[i] * t2) + y0) - Y[i]) / sigma[i];
    val += dYi * dYi;
  }
  return val;
}
int expd2_df(const gsl_vector *x, void *params, gsl_matrix *J) {
  size_t n = ((struct FitData *)params)->n;
  double *X = ((struct FitData *)params)->X;
  double *sigma = ((struct FitData *)params)->sigma;
  double A1 = gsl_vector_get(x, 0);
  double l1 = gsl_vector_get(x, 1);
  double A2 = gsl_vector_get(x, 2);
  double l2 = gsl_vector_get(x, 3);
  size_t i;
  for (i = 0; i < n; i++) {
    /* Jacobian matrix J(i,j) = dfi / dxj, */
    /* where fi = (Yi - yi)/sigma[i],      */
    /*       Yi = A1 * exp(-xi*l1) + A2 * exp(-xi*l2) +y0  */
    /* and the xj are the parameters (A1,l1,A2,l2,y0) */
    double s = sigma[i];
    double t = X[i];
    double e1 = exp(-t * l1) / s;
    double e2 = exp(-t * l2) / s;
    gsl_matrix_set(J, i, 0, e1);
    gsl_matrix_set(J, i, 1, -t * A1 * e1);
    gsl_matrix_set(J, i, 2, e2);
    gsl_matrix_set(J, i, 3, -t * A2 * e2);
    gsl_matrix_set(J, i, 4, 1 / s);
  }
  return GSL_SUCCESS;
}
int expd2_fdf(const gsl_vector *x, void *params, gsl_vector *f, gsl_matrix *J) {
  expd2_f(x, params, f);
  expd2_df(x, params, J);
  return GSL_SUCCESS;
}
int exp_f(const gsl_vector *x, void *params, gsl_vector *f) {
  size_t n = ((struct FitData *)params)->n;
  double *X = ((struct FitData *)params)->X;
  double *Y = ((struct FitData *)params)->Y;
  double *sigma = ((struct FitData *)params)->sigma;
  double A = gsl_vector_get(x, 0);
  double lambda = gsl_vector_get(x, 1);
  double b = gsl_vector_get(x, 2);
  size_t i;
  for (i = 0; i < n; i++) {
    double Yi = A * exp(-lambda * X[i]) + b;
    gsl_vector_set(f, i, (Yi - Y[i]) / sigma[i]);
  }
  return GSL_SUCCESS;
}
double exp_d(const gsl_vector *x, void *params) {
  size_t n = ((struct FitData *)params)->n;
  double *X = ((struct FitData *)params)->X;
  double *Y = ((struct FitData *)params)->Y;
  double *sigma = ((struct FitData *)params)->sigma;
  double A = gsl_vector_get(x, 0);
  double lambda = gsl_vector_get(x, 1);
  double b = gsl_vector_get(x, 2);
  size_t i;
  double val = 0;
  for (i = 0; i < n; i++) {
    double dYi = ((A * exp(-lambda * X[i]) + b) - Y[i]) / sigma[i];
    val += dYi * dYi;
  }
  return val;
}
int exp_df(const gsl_vector *x, void *params, gsl_matrix *J) {
  size_t n = ((struct FitData *)params)->n;
  double *X = ((struct FitData *)params)->X;
  double *sigma = ((struct FitData *)params)->sigma;
  double A = gsl_vector_get(x, 0);
  double lambda = gsl_vector_get(x, 1);
  size_t i;
  for (i = 0; i < n; i++) {
    /* Jacobian matrix J(i,j) = dfi / dxj, */
    /* where fi = (Yi - yi)/sigma[i],      */
    /*       Yi = A * exp(-lambda * i) + b  */
    /* and the xj are the parameters (A,lambda,b) */
    double t = X[i];
    double s = sigma[i];
    double e = exp(-lambda * t);
    gsl_matrix_set(J, i, 0, e / s);
    gsl_matrix_set(J, i, 1, -t * A * e / s);
    gsl_matrix_set(J, i, 2, 1 / s);
  }
  return GSL_SUCCESS;
}
int exp_fdf(const gsl_vector *x, void *params, gsl_vector *f, gsl_matrix *J) {
  exp_f(x, params, f);
  exp_df(x, params, J);
  return GSL_SUCCESS;
}

int gauss_f(const gsl_vector *x, void *params, gsl_vector *f) {
  size_t n = ((struct FitData *)params)->n;
  double *X = ((struct FitData *)params)->X;
  double *Y = ((struct FitData *)params)->Y;
  double *sigma = ((struct FitData *)params)->sigma;
  double Y0 = gsl_vector_get(x, 0);
  double A = gsl_vector_get(x, 1);
  double C = gsl_vector_get(x, 2);
  double w = gsl_vector_get(x, 3);
  size_t i;
  for (i = 0; i < n; i++) {
    double diff = X[i] - C;
    double Yi = A * exp(-0.5 * diff * diff / (w * w)) + Y0;
    gsl_vector_set(f, i, (Yi - Y[i]) / sigma[i]);
  }
  return GSL_SUCCESS;
}
double gauss_d(const gsl_vector *x, void *params) {
  size_t n = ((struct FitData *)params)->n;
  double *X = ((struct FitData *)params)->X;
  double *Y = ((struct FitData *)params)->Y;
  double *sigma = ((struct FitData *)params)->sigma;
  double Y0 = gsl_vector_get(x, 0);
  double A = gsl_vector_get(x, 1);
  double C = gsl_vector_get(x, 2);
  double w = gsl_vector_get(x, 3);
  size_t i;
  double val = 0;
  for (i = 0; i < n; i++) {
    double diff = X[i] - C;
    double dYi =
        ((A * exp(-0.5 * diff * diff / (w * w)) + Y0) - Y[i]) / sigma[i];
    val += dYi * dYi;
  }
  return val;
}
int gauss_df(const gsl_vector *x, void *params, gsl_matrix *J) {
  size_t n = ((struct FitData *)params)->n;
  double *X = ((struct FitData *)params)->X;
  double *sigma = ((struct FitData *)params)->sigma;
  double A = gsl_vector_get(x, 1);
  double C = gsl_vector_get(x, 2);
  double w = gsl_vector_get(x, 3);
  size_t i;
  for (i = 0; i < n; i++) {
    /* Jacobian matrix J(i,j) = dfi / dxj,	 */
    /* where fi = Yi - yi,					*/
    /* Yi = y=A*exp[-(Xi-xc)^2/(2*w*w)]+B		*/
    /* and the xj are the parameters (B,A,C,w) */
    double s = sigma[i];
    double diff = X[i] - C;
    double e = exp(-0.5 * diff * diff / (w * w)) / s;
    gsl_matrix_set(J, i, 0, 1 / s);
    gsl_matrix_set(J, i, 1, e);
    gsl_matrix_set(J, i, 2, diff * A * e / (w * w));
    gsl_matrix_set(J, i, 3, diff * diff * A * e / (w * w * w));
  }
  return GSL_SUCCESS;
}
int gauss_fdf(const gsl_vector *x, void *params, gsl_vector *f, gsl_matrix *J) {
  gauss_f(x, params, f);
  gauss_df(x, params, J);
  return GSL_SUCCESS;
}
int gauss_multi_peak_f(const gsl_vector *x, void *params, gsl_vector *f) {
  size_t n = ((struct FitData *)params)->n;
  size_t p = ((struct FitData *)params)->p;
  double *X = ((struct FitData *)params)->X;
  double *Y = ((struct FitData *)params)->Y;
  double *sigma = ((struct FitData *)params)->sigma;
  size_t peaks = (p - 1) / 3;
  double *a = new double[peaks];
  double *xc = new double[peaks];
  double *w2 = new double[peaks];
  double offset = gsl_vector_get(x, p - 1);
  size_t i, j;
  for (i = 0; i < peaks; i++) {
    xc[i] = gsl_vector_get(x, 3 * i + 1);
    double wi = gsl_vector_get(x, 3 * i + 2);
    a[i] = sqrt(M_2_PI) * gsl_vector_get(x, 3 * i) / wi;
    w2[i] = wi * wi;
  }
  for (i = 0; i < n; i++) {
    double res = 0;
    for (j = 0; j < peaks; j++) {
      double diff = X[i] - xc[j];
      res += a[j] * exp(-2 * diff * diff / w2[j]);
    }
    gsl_vector_set(f, i, (res + offset - Y[i]) / sigma[i]);
  }
  delete[] a;
  delete[] xc;
  delete[] w2;
  return GSL_SUCCESS;
}
double gauss_multi_peak_d(const gsl_vector *x, void *params) {
  size_t n = ((struct FitData *)params)->n;
  size_t p = ((struct FitData *)params)->p;
  double *X = ((struct FitData *)params)->X;
  double *Y = ((struct FitData *)params)->Y;
  double *sigma = ((struct FitData *)params)->sigma;
  size_t peaks = (p - 1) / 3;
  double *a = new double[peaks];
  double *xc = new double[peaks];
  double *w2 = new double[peaks];
  double offset = gsl_vector_get(x, p - 1);
  size_t i, j;
  double val = 0;
  for (i = 0; i < peaks; i++) {
    xc[i] = gsl_vector_get(x, 3 * i + 1);
    double wi = gsl_vector_get(x, 3 * i + 2);
    a[i] = sqrt(M_2_PI) * gsl_vector_get(x, 3 * i) / wi;
    w2[i] = wi * wi;
  }

  for (i = 0; i < n; i++) {
    double res = 0;
    for (j = 0; j < peaks; j++) {
      double diff = X[i] - xc[j];
      res += a[j] * exp(-2 * diff * diff / w2[j]);
    }
    double t = (res + offset - Y[i]) / sigma[i];
    val += t * t;
  }
  delete[] a;
  delete[] xc;
  delete[] w2;
  return val;
}
int gauss_multi_peak_df(const gsl_vector *x, void *params, gsl_matrix *J) {
  size_t n = ((struct FitData *)params)->n;
  size_t p = ((struct FitData *)params)->p;
  double *X = ((struct FitData *)params)->X;
  double *sigma = ((struct FitData *)params)->sigma;
  size_t peaks = (p - 1) / 3;
  double *a = new double[peaks];
  double *xc = new double[peaks];
  double *w = new double[peaks];
  size_t i, j;
  for (i = 0; i < peaks; i++) {
    a[i] = gsl_vector_get(x, 3 * i);
    xc[i] = gsl_vector_get(x, 3 * i + 1);
    w[i] = gsl_vector_get(x, 3 * i + 2);
  }
  for (i = 0; i < n; i++) {
    double s = sigma[i];
    for (j = 0; j < peaks; j++) {
      double diff = X[i] - xc[j];
      double w2 = w[j] * w[j];
      double e = sqrt(M_2_PI) / s * exp(-2 * diff * diff / w2);
      gsl_matrix_set(J, i, 3 * j, e / w[j]);
      gsl_matrix_set(J, i, 3 * j + 1, 4 * diff * a[j] * e / (w2 * w[j]));
      gsl_matrix_set(J, i, 3 * j + 2,
                     a[j] / w2 * e * (4 * diff * diff / w2 - 1));
    }
    gsl_matrix_set(J, i, p - 1, 1.0 / s);
  }
  delete[] a;
  delete[] xc;
  delete[] w;
  return GSL_SUCCESS;
}
int gauss_multi_peak_fdf(const gsl_vector *x, void *params, gsl_vector *f,
                         gsl_matrix *J) {
  gauss_multi_peak_f(x, params, f);
  gauss_multi_peak_df(x, params, J);
  return GSL_SUCCESS;
}
int lorentz_multi_peak_f(const gsl_vector *x, void *params, gsl_vector *f) {
  size_t n = ((struct FitData *)params)->n;
  size_t p = ((struct FitData *)params)->p;
  double *X = ((struct FitData *)params)->X;
  double *Y = ((struct FitData *)params)->Y;
  double *sigma = ((struct FitData *)params)->sigma;
  size_t peaks = (p - 1) / 3;
  double *a = new double[peaks];
  double *xc = new double[peaks];
  double *w = new double[peaks];
  double offset = gsl_vector_get(x, p - 1);
  size_t i, j;
  for (i = 0; i < peaks; i++) {
    a[i] = gsl_vector_get(x, 3 * i);
    xc[i] = gsl_vector_get(x, 3 * i + 1);
    w[i] = gsl_vector_get(x, 3 * i + 2);
  }
  for (i = 0; i < n; i++) {
    double res = 0;
    for (j = 0; j < peaks; j++) {
      double diff = X[i] - xc[j];
      res += a[j] * w[j] / (4 * diff * diff + w[j] * w[j]);
    }
    gsl_vector_set(f, i, (M_2_PI * res + offset - Y[i]) / sigma[i]);
  }
  delete[] a;
  delete[] xc;
  delete[] w;
  return GSL_SUCCESS;
}
double lorentz_multi_peak_d(const gsl_vector *x, void *params) {
  size_t n = ((struct FitData *)params)->n;
  size_t p = ((struct FitData *)params)->p;
  double *X = ((struct FitData *)params)->X;
  double *Y = ((struct FitData *)params)->Y;
  double *sigma = ((struct FitData *)params)->sigma;
  size_t peaks = (p - 1) / 3;
  double *a = new double[peaks];
  double *xc = new double[peaks];
  double *w = new double[peaks];
  double offset = gsl_vector_get(x, p - 1);
  double val = 0;
  for (size_t i = 0; i < peaks; i++) {
    a[i] = gsl_vector_get(x, 3 * i);
    xc[i] = gsl_vector_get(x, 3 * i + 1);
    w[i] = gsl_vector_get(x, 3 * i + 2);
  }
  for (size_t i = 0; i < n; i++) {
    double res = 0;
    for (size_t j = 0; j < peaks; j++) {
      double diff = X[i] - xc[j];
      res += a[j] * w[j] / (4 * diff * diff + w[j] * w[j]);
    }
    double t = (M_2_PI * res + offset - Y[i]) / sigma[i];
    val += t * t;
  }
  delete[] a;
  delete[] xc;
  delete[] w;
  return val;
}
int lorentz_multi_peak_df(const gsl_vector *x, void *params, gsl_matrix *J) {
  size_t n = ((struct FitData *)params)->n;
  size_t p = ((struct FitData *)params)->p;
  double *X = ((struct FitData *)params)->X;
  double *sigma = ((struct FitData *)params)->sigma;
  size_t peaks = (p - 1) / 3;
  double *a = new double[peaks];
  double *xc = new double[peaks];
  double *w = new double[peaks];
  size_t i, j;
  for (i = 0; i < peaks; i++) {
    a[i] = gsl_vector_get(x, 3 * i);
    xc[i] = gsl_vector_get(x, 3 * i + 1);
    w[i] = gsl_vector_get(x, 3 * i + 2);
  }
  for (i = 0; i < n; i++) {
    double s = sigma[i];
    for (j = 0; j < peaks; j++) {
      double diff = X[i] - xc[j];
      double diff2 = diff * diff;
      double w2 = w[j] * w[j];
      double num = 1.0 / (4 * diff2 + w2);
      double num2 = num * num;
      double den = 4 * diff2 - w2;
      gsl_matrix_set(J, i, 3 * j, M_2_PI * w[j] * num / s);
      gsl_matrix_set(J, i, 3 * j + 1,
                     M_2_PI * 8 * diff * a[j] * w[j] * num2 / s);
      gsl_matrix_set(J, i, 3 * j + 2, M_2_PI * den * a[j] * num2 / s);
    }
    gsl_matrix_set(J, i, p - 1, 1.0 / s);
  }
  delete[] a;
  delete[] xc;
  delete[] w;
  return GSL_SUCCESS;
}
int lorentz_multi_peak_fdf(const gsl_vector *x, void *params, gsl_vector *f,
                           gsl_matrix *J) {
  lorentz_multi_peak_f(x, params, f);
  lorentz_multi_peak_df(x, params, J);
  return GSL_SUCCESS;
}
int user_f(const gsl_vector *x, void *params, gsl_vector *f) {
  size_t n = ((struct FitData *)params)->n;
  size_t p = ((struct FitData *)params)->p;
  double *X = ((struct FitData *)params)->X;
  double *Y = ((struct FitData *)params)->Y;
  double *sigma = ((struct FitData *)params)->sigma;
  const char *function = ((struct FitData *)params)->function;
  QString names = (QString)((struct FitData *)params)->names;
  QStringList parNames = names.split(",", QString::SkipEmptyParts);
  MyParser parser;
  try {
    double *parameters = new double[p];
    double xvar;
    parser.DefineVar("x", &xvar);
    for (int i = 0; i < (int)p; i++) {
      parameters[i] = gsl_vector_get(x, i);
      parser.DefineVar(parNames[i].toAscii().constData(), &parameters[i]);
    }
    parser.SetExpr(function);
    for (int j = 0; j < (int)n; j++) {
      xvar = X[j];
      gsl_vector_set(f, j, (parser.Eval() - Y[j]) / sigma[j]);
    }
    delete[] parameters;
  } catch (mu::ParserError &e) {
    QMessageBox::critical(nullptr, "MantidPlot - Input function error",
                          QString::fromStdString(e.GetMsg()));
    return GSL_EINVAL;
  }
  return GSL_SUCCESS;
}
double user_d(const gsl_vector *x, void *params) {
  size_t n = ((struct FitData *)params)->n;
  size_t p = ((struct FitData *)params)->p;
  double *X = ((struct FitData *)params)->X;
  double *Y = ((struct FitData *)params)->Y;
  double *sigma = ((struct FitData *)params)->sigma;
  const char *function = ((struct FitData *)params)->function;
  QString names = (QString)((struct FitData *)params)->names;
  QStringList parNames = names.split(",", QString::SkipEmptyParts);
  double val = 0;
  MyParser parser;
  try {
    double *parameters = new double[p];
    double xvar;
    parser.DefineVar("x", &xvar);
    for (int i = 0; i < (int)p; i++) {
      parameters[i] = gsl_vector_get(x, i);
      parser.DefineVar(parNames[i].toAscii().constData(), &parameters[i]);
    }
    parser.SetExpr(function);
    for (int j = 0; j < (int)n; j++) {
      xvar = X[j];
      double t = (parser.Eval() - Y[j]) / sigma[j];
      val += t * t;
    }
    delete[] parameters;
  } catch (mu::ParserError &e) {
    QMessageBox::critical(nullptr, "MantidPlot - Input function error",
                          QString::fromStdString(e.GetMsg()));
    return GSL_EINVAL;
  }
  return val;
}
int user_df(const gsl_vector *x, void *params, gsl_matrix *J) {
  size_t n = ((struct FitData *)params)->n;
  size_t p = ((struct FitData *)params)->p;
  double *X = ((struct FitData *)params)->X;
  double *sigma = ((struct FitData *)params)->sigma;
  const char *function = ((struct FitData *)params)->function;
  QString names = (QString)((struct FitData *)params)->names;
  QStringList parNames = names.split(",", QString::SkipEmptyParts);
  try {
    double *param = new double[p];
    MyParser parser;
    double xvar;
    parser.DefineVar("x", &xvar);
    for (int k = 0; k < (int)p; k++) {
      param[k] = gsl_vector_get(x, k);
      parser.DefineVar(parNames[k].toAscii().constData(), &param[k]);
    }
    parser.SetExpr(function);
    for (int i = 0; i < (int)n; i++) {
      xvar = X[i];
      for (int j = 0; j < (int)p; j++)
        gsl_matrix_set(J, i, j,
                       1 / sigma[i] * parser.Diff(&param[j], param[j]));
    }
    delete[] param;
  } catch (mu::ParserError &) {
    return GSL_EINVAL;
  }
  return GSL_SUCCESS;
}
int user_fdf(const gsl_vector *x, void *params, gsl_vector *f, gsl_matrix *J) {
  user_f(x, params, f);
  user_df(x, params, J);
  return GSL_SUCCESS;
}
int boltzmann_f(const gsl_vector *x, void *params, gsl_vector *f) {
  size_t n = ((struct FitData *)params)->n;
  double *X = ((struct FitData *)params)->X;
  double *Y = ((struct FitData *)params)->Y;
  double *sigma = ((struct FitData *)params)->sigma;
  double A1 = gsl_vector_get(x, 0);
  double A2 = gsl_vector_get(x, 1);
  double x0 = gsl_vector_get(x, 2);
  double dx = gsl_vector_get(x, 3);
  size_t i;
  for (i = 0; i < n; i++) {
    double Yi = (A1 - A2) / (1 + exp((X[i] - x0) / dx)) + A2;
    gsl_vector_set(f, i, (Yi - Y[i]) / sigma[i]);
  }
  return GSL_SUCCESS;
}
double boltzmann_d(const gsl_vector *x, void *params) {
  size_t n = ((struct FitData *)params)->n;
  double *X = ((struct FitData *)params)->X;
  double *Y = ((struct FitData *)params)->Y;
  double *sigma = ((struct FitData *)params)->sigma;
  double A1 = gsl_vector_get(x, 0);
  double A2 = gsl_vector_get(x, 1);
  double x0 = gsl_vector_get(x, 2);
  double dx = gsl_vector_get(x, 3);
  size_t i;
  double val = 0;
  for (i = 0; i < n; i++) {
    double dYi =
        ((A1 - A2) / (1 + exp((X[i] - x0) / dx)) + A2 - Y[i]) / sigma[i];
    val += dYi * dYi;
  }
  return val;
}
int boltzmann_df(const gsl_vector *x, void *params, gsl_matrix *J) {
  size_t n = ((struct FitData *)params)->n;
  double *X = ((struct FitData *)params)->X;
  double *sigma = ((struct FitData *)params)->sigma;
  double A1 = gsl_vector_get(x, 0);
  double A2 = gsl_vector_get(x, 1);
  double x0 = gsl_vector_get(x, 2);
  double dx = gsl_vector_get(x, 3);
  size_t i;
  for (i = 0; i < n; i++) {
    /* Jacobian matrix J(i,j) = dfi / dxj,		*/
    /* where fi = Yi - yi,						*/
    /* Yi = (A1-A2)/(1+exp((X[i]-x0)/dx)) + A2	*/
    /* and the xj are the parameters (A1,A2,x0,dx)*/
    double s = sigma[i];
    double diff = X[i] - x0;
    double e = exp(diff / dx);
    double r = 1 / (1 + e);
    double aux = (A1 - A2) * e * r * r / (dx * s);
    gsl_matrix_set(J, i, 0, r / s);
    gsl_matrix_set(J, i, 1, (1 - r) / s);
    gsl_matrix_set(J, i, 2, aux);
    gsl_matrix_set(J, i, 3, aux * diff / dx);
  }
  return GSL_SUCCESS;
}
int boltzmann_fdf(const gsl_vector *x, void *params, gsl_vector *f,
                  gsl_matrix *J) {
  boltzmann_f(x, params, f);
  boltzmann_df(x, params, J);
  return GSL_SUCCESS;
}

int logistic_f(const gsl_vector *x, void *params, gsl_vector *f) {
  size_t n = ((struct FitData *)params)->n;
  double *X = ((struct FitData *)params)->X;
  double *Y = ((struct FitData *)params)->Y;
  double *sigma = ((struct FitData *)params)->sigma;

  double A1 = gsl_vector_get(x, 0);
  double A2 = gsl_vector_get(x, 1);
  double x0 = gsl_vector_get(x, 2);
  double p = gsl_vector_get(x, 3);
  size_t i;
  for (i = 0; i < n; i++) {
    double Yi = (A1 - A2) / (1 + pow(X[i] / x0, p)) + A2;
    gsl_vector_set(f, i, (Yi - Y[i]) / sigma[i]);
  }

  return GSL_SUCCESS;
}

double logistic_d(const gsl_vector *x, void *params) {
  size_t n = ((struct FitData *)params)->n;
  double *X = ((struct FitData *)params)->X;
  double *Y = ((struct FitData *)params)->Y;
  double *sigma = ((struct FitData *)params)->sigma;

  double A1 = gsl_vector_get(x, 0);
  double A2 = gsl_vector_get(x, 1);
  double x0 = gsl_vector_get(x, 2);
  double p = gsl_vector_get(x, 3);
  size_t i;
  double val = 0;
  for (i = 0; i < n; i++) {
    double dYi = ((A1 - A2) / (1 + pow(X[i] / x0, p)) + A2 - Y[i]) / sigma[i];
    val += dYi * dYi;
  }
  return val;
}

int logistic_df(const gsl_vector *x, void *params, gsl_matrix *J) {
  size_t n = ((struct FitData *)params)->n;
  double *X = ((struct FitData *)params)->X;
  double *sigma = ((struct FitData *)params)->sigma;

  double A1 = gsl_vector_get(x, 0);
  double A2 = gsl_vector_get(x, 1);
  double x0 = gsl_vector_get(x, 2);
  double p = gsl_vector_get(x, 3);
  size_t i;
  for (i = 0; i < n; i++) {
    /* Jacobian matrix J(i,j) = dfi / dxj,		*/
    /* where fi = Yi - yi,						*/
    /* Yi = (A1-A2)/(1+(X[i]/x0)^p)) + A2	*/
    /* and the xj are the parameters (A1,A2,x0,p)*/
    double s = sigma[i];
    double rap = X[i] / x0;
    double r = 1 / (1 + pow(rap, p));
    double aux = (A1 - A2) * r * r * pow(rap, p);
    gsl_matrix_set(J, i, 0, r / s);
    gsl_matrix_set(J, i, 1, (1 - r) / s);
    gsl_matrix_set(J, i, 2, aux * p / (x0 * s));
    gsl_matrix_set(J, i, 3, -aux * log(rap) / s);
  }
  return GSL_SUCCESS;
}

int logistic_fdf(const gsl_vector *x, void *params, gsl_vector *f,
                 gsl_matrix *J) {
  logistic_f(x, params, f);
  logistic_df(x, params, J);
  return GSL_SUCCESS;
}
