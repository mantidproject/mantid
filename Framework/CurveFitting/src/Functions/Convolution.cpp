// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/Convolution.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidCurveFitting/Functions/DeltaFunction.h"

#include <algorithm>
#include <cmath>
#include <functional>

#include <gsl/gsl_errno.h>
#include <gsl/gsl_fft_halfcomplex.h>
#include <gsl/gsl_fft_real.h>

#include <fstream>
#include <sstream>

namespace {
const double tolerance{0.02};
}

namespace Mantid {
namespace CurveFitting {
namespace Functions {

using namespace CurveFitting;

using namespace Kernel;

using namespace API;

DECLARE_FUNCTION(Convolution)

/// Constructor
Convolution::Convolution() {
  declareAttribute("FixResolution", Attribute(true));
  setAttributeValue("NumDeriv", true);
}

void Convolution::init() {}

void Convolution::functionDeriv(const FunctionDomain &domain,
                                Jacobian &jacobian) {
  calNumericalDeriv(domain, jacobian);
}

void Convolution::setAttribute(const std::string &attName,
                               const IFunction::Attribute &att) {
  // if the resolution is there fix/unfix its parameters according to the
  // attribute value
  if (attName == "FixResolution" && nFunctions() > 0) {
    bool fixRes = att.asBool();
    auto &f = *getFunction(0);
    for (size_t i = 0; i < f.nParams(); i++) {
      if (fixRes) {
        f.fix(i);
      } else {
        f.unfix(i);
      }
    }
  }
  CompositeFunction::setAttribute(attName, att);
}

namespace {
// anonymous namespace for local definitions

// A struct incapsulating workspaces for real fft
struct RealFFTWorkspace {
  explicit RealFFTWorkspace(size_t nData)
      : workspace(gsl_fft_real_workspace_alloc(nData)),
        wavetable(gsl_fft_real_wavetable_alloc(nData)) {}
  ~RealFFTWorkspace() {
    gsl_fft_real_wavetable_free(wavetable);
    gsl_fft_real_workspace_free(workspace);
  }
  gsl_fft_real_workspace *workspace;
  gsl_fft_real_wavetable *wavetable;
};
} // namespace

/**
 * Calculates convolution of the two member functions. Switches from FFT mode
 * to direct mode if the domain is not symmetric with respect to the
 * inversion E --> -E.
 * @param domain :: space on which the function acts
 * @param values :: buffer to store the values returned by the function after
 * acting on the domain.
 */
void Convolution::function(const FunctionDomain &domain,
                           FunctionValues &values) const {
  if (nFunctions() == 0) {
    values.zeroCalculated();
    return;
  }
  const auto &d1d = dynamic_cast<const FunctionDomain1D &>(domain);
  const size_t nData = domain.size();
  const double *xValues = d1d.getPointerAt(0);
  double dx =
      (xValues[nData - 1] - xValues[0]) / static_cast<double>((nData - 1));
  // positive x-values:
  auto ixP = static_cast<size_t>(xValues[nData - 1] / dx);
  auto ixN = nData - ixP - 1; // negative x-values (ixP+ixN=nData-1)

  // determine wether to use FFT or Direct calculations
  int assymmetry = abs(static_cast<int>(ixP - ixN));
  if (xValues[0] * xValues[nData - 1] < 0 &&
      assymmetry > tolerance * static_cast<double>(ixP + ixN)) {
    functionDirectMode(domain, values);
  } else {
    functionFFTMode(domain, values);
  }
}

/**
 * Calculates convolution of the two member functions when the
 * domain is symmetric with respect to inversion E --> -E.
 * @param domain :: space on which the function acts
 * @param values :: buffer to store the values returned by the function after
 * acting on the domain.
 */
void Convolution::functionFFTMode(const FunctionDomain &domain,
                                  FunctionValues &values) const {
  const auto &d1d = dynamic_cast<const FunctionDomain1D &>(domain);
  size_t nData = domain.size();
  const double *xValues = d1d.getPointerAt(0);
  refreshResolution();
  RealFFTWorkspace workspace(nData);
  int n2 = static_cast<int>(nData) / 2;
  bool odd = n2 * 2 != static_cast<int>(nData);
  if (m_resolution.empty()) {
    m_resolution.resize(nData);
    // the resolution must be defined on interval -L < xr < L, L ==
    // (xValues[nData-1] - xValues[0]) / 2
    std::vector<double> xr(nData);
    double dx =
        (xValues[nData - 1] - xValues[0]) / static_cast<double>((nData - 1));
    // make sure that xr[nData/2] == 0.0
    xr[n2] = 0.0;
    for (int i = 1; i < n2; i++) {
      double x = i * dx;
      xr[n2 + i] = x;
      xr[n2 - i] = -x;
    }

    xr[0] = -n2 * dx;
    if (odd)
      xr[nData - 1] = -xr[0];

    IFunction1D_sptr fun =
        boost::dynamic_pointer_cast<IFunction1D>(getFunction(0));
    if (!fun) {
      throw std::runtime_error("Convolution can work only with IFunction1D");
    }
    fun->function1D(m_resolution.data(), xr.data(), nData);

    // rotate the data to produce the right transform
    if (odd) {
      double tmp = m_resolution[nData - 1];
      for (int i = n2 - 1; i >= 0; i--) {
        m_resolution[n2 + i + 1] = m_resolution[i];
        m_resolution[i] = m_resolution[n2 + i];
      }
      m_resolution[n2] = tmp;
    } else {
      for (int i = 0; i < n2; i++) {
        double tmp = m_resolution[i];
        m_resolution[i] = m_resolution[n2 + i];
        m_resolution[n2 + i] = tmp;
      }
    }
    gsl_fft_real_transform(m_resolution.data(), 1, nData, workspace.wavetable,
                           workspace.workspace);
    std::transform(m_resolution.begin(), m_resolution.end(),
                   m_resolution.begin(),
                   std::bind2nd(std::multiplies<double>(), dx));
  }

  // Now m_resolution contains fourier transform of the resolution

  if (nFunctions() == 1) {
    // return the resolution transform for testing
    double dx = 1.; // nData > 1? xValues[1] - xValues[0]: 1.;
    std::transform(m_resolution.begin(), m_resolution.end(),
                   values.getPointerToCalculated(0),
                   std::bind2nd(std::multiplies<double>(), dx));
    return;
  }

  IFunction1D_sptr resolution =
      boost::dynamic_pointer_cast<IFunction1D>(getFunction(0));

  // check for delta functions
  std::vector<boost::shared_ptr<DeltaFunction>> dltFuns;
  double dltF = 0;
  bool deltaFunctionsOnly = false;
  bool deltaShifted = false;
  CompositeFunction_sptr cf =
      boost::dynamic_pointer_cast<CompositeFunction>(getFunction(1));
  if (cf) {
    dltFuns.reserve(cf->nFunctions());
    for (size_t i = 0; i < cf->nFunctions(); ++i) {
      auto df = boost::dynamic_pointer_cast<DeltaFunction>(cf->getFunction(i));
      if (df) {
        dltFuns.push_back(df);
        if (df->getParameter("Centre") != 0.0) {
          deltaShifted = true;
        }
        dltF += df->getParameter("Height") * df->HeightPrefactor();
      }
    }
    if (dltFuns.size() == cf->nFunctions()) {
      // all delta functions - return scaled resolution
      deltaFunctionsOnly = true;
    }
  } else if (auto df =
                 boost::dynamic_pointer_cast<DeltaFunction>(getFunction(1))) {
    // single delta function - return scaled resolution
    deltaFunctionsOnly = true;
    dltFuns.push_back(df);
    if (df->getParameter("Centre") != 0.0) {
      deltaShifted = true;
    }
    dltF = df->getParameter("Height") * df->HeightPrefactor();
  }

  // out points to the calculated values in values
  double *out = values.getPointerToCalculated(0);

  if (!deltaFunctionsOnly) {
    // Transform the model function
    getFunction(1)->function(domain, values);
    gsl_fft_real_transform(out, 1, nData, workspace.wavetable,
                           workspace.workspace);

    // Fourier transform is integration - multiply by the step in the
    // integration variable
    double dx = nData > 1 ? xValues[1] - xValues[0] : 1.;
    std::transform(out, out + nData, out,
                   std::bind2nd(std::multiplies<double>(), dx));

    // now out contains fourier transform of the model function

    HalfComplex res(m_resolution.data(), nData);
    HalfComplex fun(out, nData);

    // Multiply transforms of the resolution and model functions
    // Result is stored in fun
    for (size_t i = 0; i <= res.size(); i++) {
      // complex multiplication
      double res_r = res.real(i);
      double res_i = res.imag(i);
      double fun_r = fun.real(i);
      double fun_i = fun.imag(i);
      fun.set(i, res_r * fun_r - res_i * fun_i, res_r * fun_i + res_i * fun_r);
    }

    // Inverse fourier transform of fun
    gsl_fft_halfcomplex_wavetable *wavetable_r =
        gsl_fft_halfcomplex_wavetable_alloc(nData);
    gsl_fft_halfcomplex_inverse(out, 1, nData, wavetable_r,
                                workspace.workspace);
    gsl_fft_halfcomplex_wavetable_free(wavetable_r);

    // Inverse fourier transform is integration - multiply by the step in the
    // integration variable
    dx = nData > 1 ? 1. / (xValues[1] - xValues[0]) : 1.;
    std::transform(out, out + nData, out,
                   std::bind2nd(std::multiplies<double>(), dx));
  } else {
    values.zeroCalculated();
  }

  if (dltF != 0.0 && !deltaShifted) {
    // If model contains any delta functions their effect is addition of scaled
    // resolution
    std::vector<double> tmp(nData);
    resolution->function1D(tmp.data(), xValues, nData);
    std::transform(tmp.begin(), tmp.end(), tmp.begin(),
                   std::bind2nd(std::multiplies<double>(), dltF));
    std::transform(out, out + nData, tmp.data(), out, std::plus<double>());
  } else if (!dltFuns.empty()) {
    std::vector<double> x(nData);
    for (const auto &df : dltFuns) {
      double shift = -df->getParameter("Centre");
      dltF = df->getParameter("Height") * df->HeightPrefactor();
      std::transform(xValues, xValues + nData, x.data(),
                     std::bind2nd(std::plus<double>(), shift));
      std::vector<double> tmp(nData);
      resolution->function1D(tmp.data(), x.data(), nData);
      std::transform(tmp.begin(), tmp.end(), tmp.begin(),
                     std::bind2nd(std::multiplies<double>(), dltF));
      std::transform(out, out + nData, tmp.data(), out, std::plus<double>());
    }
  }

} // end of Convolution::functionFFTMode

/**
 * Calculates convolution of the two member functions when the
 * domain is not symmetric with respect to inversion E --> -E.
 * The direct mode is slower than the FFT mode. This is meant to be a temporary
 * method until the need for a faster computation of asymetric domain arises.
 * @param domain :: space on which the function acts
 * @param values :: buffer to store the values returned by the function after
 * acting on the domain.
 */
void Convolution::functionDirectMode(const FunctionDomain &domain,
                                     FunctionValues &values) const {
  const auto &d1d = dynamic_cast<const FunctionDomain1D &>(domain);
  const size_t nData = domain.size();
  const double *xValues = d1d.getPointerAt(0);
  double dx =
      (xValues[nData - 1] - xValues[0]) / static_cast<double>((nData - 1));
  auto ixP = static_cast<size_t>(xValues[nData - 1] / dx); // positive
                                                           // x-values
  auto ixN = nData - ixP - 1; // negative x-values (ixP+ixN=nData-1)

  refreshResolution();

  // double the domain where to evaluate the convolution. Guarantees complete
  // overlap betwen convolution and signal in the original range.
  const size_t mData = nData + ixN + ixP; // equal to 2*nData-1
  std::vector<double> xValuesExtdVec(mData);
  double *xValuesExtd = xValuesExtdVec.data();
  Mantid::API::FunctionDomain1DView domainExtd(xValuesExtd, mData);
  double Dx = dx * static_cast<double>(ixN + ixP);
  for (size_t i = 0; i < mData; i++) {
    xValuesExtd[i] = -Dx + static_cast<double>(i) * dx;
  }

  // Fill m_resolution with the resolution function data
  // Lines 341-349 is duplicated in functionFFTmode. To be cleanup
  // in issue 16064
  IFunction1D_sptr resolution =
      boost::dynamic_pointer_cast<IFunction1D>(getFunction(0));
  if (!resolution) {
    throw std::runtime_error("Convolution can work only with IFunction1D");
  }
  if (m_resolution.empty()) {
    m_resolution.resize(nData);
  }
  resolution->function1D(m_resolution.data(), xValues, nData);

  // Reverse the axis of the resolution data
  std::reverse(m_resolution.begin(), m_resolution.end());

  // check for delta functions
  std::vector<boost::shared_ptr<DeltaFunction>> dltFuns;
  double dltF = 0;
  bool deltaFunctionsOnly = false;
  bool deltaShifted = false;
  CompositeFunction_sptr cf =
      boost::dynamic_pointer_cast<CompositeFunction>(getFunction(1));
  if (cf) {
    dltFuns.reserve(cf->nFunctions());
    for (size_t i = 0; i < cf->nFunctions(); ++i) {
      auto df = boost::dynamic_pointer_cast<DeltaFunction>(cf->getFunction(i));
      if (df) {
        dltFuns.push_back(df);
        if (df->getParameter("Centre") != 0.0) {
          deltaShifted = true;
        }
        dltF += df->getParameter("Height") * df->HeightPrefactor();
      }
    }
    if (dltFuns.size() == cf->nFunctions()) {
      // all delta functions - return scaled resolution
      deltaFunctionsOnly = true;
    }
  } else if (auto df =
                 boost::dynamic_pointer_cast<DeltaFunction>(getFunction(1))) {
    // single delta function - return scaled resolution
    deltaFunctionsOnly = true;
    dltFuns.push_back(df);
    if (df->getParameter("Centre") != 0.0) {
      deltaShifted = true;
    }
    dltF = df->getParameter("Height") * df->HeightPrefactor();
  }

  // store the result of the convolution
  double *out = values.getPointerToCalculated(0);

  if (!deltaFunctionsOnly) {
    // Evaluate the model on the extended domain
    Mantid::API::FunctionValues valuesExtd(domainExtd);
    getFunction(1)->function(domainExtd, valuesExtd);
    // Convolve with resolution
    double *outExt = valuesExtd.getPointerToCalculated(0);
    for (size_t i = 0; i < nData; i++) {
      double tmp{0.0};
      for (size_t j = 0; j < nData; j++) {
        tmp += outExt[i + j] * m_resolution[j];
      }
      out[i] = tmp * dx;
    }
  } else {
    values.zeroCalculated();
  }

  if (dltF != 0.0 && !deltaShifted) {
    // If model contains any delta functions their effect is addition of scaled
    // resolution
    // Lines 412-430 is duplicated in functionFFTmode. To be cleanup
    // in issue 16064
    std::vector<double> tmp(nData);
    resolution->function1D(tmp.data(), xValues, nData);
    std::transform(tmp.begin(), tmp.end(), tmp.begin(),
                   std::bind2nd(std::multiplies<double>(), dltF));
    std::transform(out, out + nData, tmp.data(), out, std::plus<double>());
  } else if (!dltFuns.empty()) {
    std::vector<double> x(nData);
    for (const auto &df : dltFuns) {
      double shift = -df->getParameter("Centre");
      dltF = df->getParameter("Height") * df->HeightPrefactor();
      std::transform(xValues, xValues + nData, x.data(),
                     std::bind2nd(std::plus<double>(), shift));
      std::vector<double> tmp(nData);
      resolution->function1D(tmp.data(), x.data(), nData);
      std::transform(tmp.begin(), tmp.end(), tmp.begin(),
                     std::bind2nd(std::multiplies<double>(), dltF));
      std::transform(out, out + nData, tmp.data(), out, std::plus<double>());
    }
  }

} // end of Convolution::functionDirectMode()

/**
 * The first function added must be the resolution.
 * The second is the convoluted (model) function. If third, fourth and so on
 * functions added they will be combined in a composite function. So the
 * Convolution always has two member functions: the resolution and the model.
 * @param f :: A pointer to the function to add
 * @return The index of the new function which will be 0 for the resolution and
 * 1 for the model
 */
size_t Convolution::addFunction(IFunction_sptr f) {
  if (boost::dynamic_pointer_cast<Convolution>(f)) {
    throw std::runtime_error("Nested convolutions are not allowed.");
  }
  if (nFunctions() == 0 && getAttribute("FixResolution").asBool()) {
    for (size_t i = 0; i < f->nParams(); i++) {
      f->fix(i);
    }
  }
  size_t iFun = 0;
  if (nFunctions() < 2) {
    iFun = CompositeFunction::addFunction(f);
  } else {
    API::IFunction_sptr f1 = getFunction(1);
    if (!f1) {
      throw std::runtime_error(
          "IFunction expected but function of another type found");
    }
    CompositeFunction_sptr cf =
        boost::dynamic_pointer_cast<CompositeFunction>(f1);
    if (cf == nullptr) {
      cf = boost::dynamic_pointer_cast<CompositeFunction>(
          API::FunctionFactory::Instance().createFunction("CompositeFunction"));
      removeFunction(1);
      cf->addFunction(f1);
      CompositeFunction::addFunction(cf);
    }
    cf->addFunction(f);
    checkFunction();
    iFun = 1;
  }
  return iFun;
}

/**
 * Make sure that the resolution is updated if this function is reused in
 * several Fits.
 */
void Convolution::setUpForFit() { m_resolution.clear(); }

/// Deletes and zeroes pointer m_resolution forsing function(...) to recalculate
/// the resolution function
void Convolution::refreshResolution() const {
  // refresh when calculation for the first time
  bool needRefreshing = m_resolution.empty();
  if (!needRefreshing) {
    // if resolution has active parameters always refresh
    IFunction &res = *getFunction(0);
    for (size_t i = 0; i < res.nParams(); ++i) {
      if (res.isActive(i)) {
        needRefreshing = true;
        break;
      }
    }
  }
  if (!needRefreshing)
    return;
  // delete fourier transform of the resolution to force its recalculation
  m_resolution.clear();
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
