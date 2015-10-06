//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Convolution.h"
#include "MantidCurveFitting/DeltaFunction.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/FunctionFactory.h"

#include <cmath>
#include <algorithm>
#include <functional>

#include <gsl/gsl_errno.h>
#include <gsl/gsl_fft_real.h>
#include <gsl/gsl_fft_halfcomplex.h>

#include <sstream>

#include <fstream>

namespace Mantid {
namespace CurveFitting {

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(Convolution)

/// Constructor
Convolution::Convolution() {
  declareAttribute("FixResolution", Attribute(true));
  setAttributeValue("NumDeriv", true);
}

/// Destructor
Convolution::~Convolution() {}

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

/**
 * Calculates convolution of the two member functions.
 * @param domain :: space on which the function acts
 * @param values :: buffer to store the values returned by the function after
 * acting on the domain.
 */
void Convolution::function(const FunctionDomain &domain,
                           FunctionValues &values) const {
  const FunctionDomain1D *d1d = dynamic_cast<const FunctionDomain1D *>(&domain);
  if (!d1d) {
    throw std::invalid_argument("Unexpected domain in Convolution");
  }

  if (nFunctions() == 0) {
    values.zeroCalculated();
    return;
  }

  size_t nData = domain.size();
  const double *xValues = d1d->getPointerAt(0);
  double *out = values.getPointerToCalculated(0);

  refreshResolution();

  gsl_fft_real_workspace *workspace = gsl_fft_real_workspace_alloc(nData);
  gsl_fft_real_wavetable *wavetable = gsl_fft_real_wavetable_alloc(nData);

  int n2 = static_cast<int>(nData) / 2;
  bool odd = n2 * 2 != static_cast<int>(nData);
  if (m_resolution.empty()) {
    m_resolution.resize(nData);
    // the resolution must be defined on interval -L < xr < L, L ==
    // (xValues[nData-1] - xValues[0]) / 2
    double *xr = new double[nData];
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
      delete[] xr;
      throw std::runtime_error("Convolution can work only with IFunction1D");
    }
    fun->function1D(m_resolution.data(), xr, nData);

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
    gsl_fft_real_transform(m_resolution.data(), 1, nData, wavetable, workspace);
    std::transform(m_resolution.begin(), m_resolution.end(),
                   m_resolution.begin(),
                   std::bind2nd(std::multiplies<double>(), dx));
    delete[] xr;
  }

  // return the resolution transform for testing
  if (nFunctions() == 1) {
    double dx = 1.; // nData > 1? xValues[1] - xValues[0]: 1.;
    std::transform(m_resolution.begin(), m_resolution.end(), out,
                   std::bind2nd(std::multiplies<double>(), dx));
    gsl_fft_real_wavetable_free(wavetable);
    gsl_fft_real_workspace_free(workspace);
    return;
  }

  IFunction1D_sptr resolution =
      boost::dynamic_pointer_cast<IFunction1D>(getFunction(0));

  // check for delta functions
  std::vector<boost::shared_ptr<DeltaFunction>> dltFuns;
  double dltF = 0;
  CompositeFunction_sptr cf =
      boost::dynamic_pointer_cast<CompositeFunction>(getFunction(1));
  if (cf) {
    for (size_t i = 0; i < cf->nFunctions(); ++i) {
      boost::shared_ptr<DeltaFunction> df =
          boost::dynamic_pointer_cast<DeltaFunction>(cf->getFunction(i));
      if (df) {
        dltFuns.push_back(df);

        dltF += df->getParameter("Height") * df->HeightPrefactor();
      }
    }
    if (dltFuns.size() ==
        cf->nFunctions()) { // all delta functions - return scaled reslution
      resolution->function1D(out, xValues, nData);
      std::transform(out, out + nData, out,
                     std::bind2nd(std::multiplies<double>(), dltF));
      gsl_fft_real_wavetable_free(wavetable);
      gsl_fft_real_workspace_free(workspace);
      return;
    }
  } else if (dynamic_cast<DeltaFunction *>(
                 getFunction(1).get())) { // single delta function - return
                                          // scaled reslution
    auto df = boost::dynamic_pointer_cast<DeltaFunction>(getFunction(1));
    resolution->function1D(out, xValues, nData);
    std::transform(
        out, out + nData, out,
        std::bind2nd(std::multiplies<double>(),
                     df->getParameter("Height") * df->HeightPrefactor()));
    gsl_fft_real_wavetable_free(wavetable);
    gsl_fft_real_workspace_free(workspace);
    return;
  }

  getFunction(1)->function(domain, values);
  gsl_fft_real_transform(out, 1, nData, wavetable, workspace);
  gsl_fft_real_wavetable_free(wavetable);

  double dx = nData > 1 ? xValues[1] - xValues[0] : 1.;
  std::transform(out, out + nData, out,
                 std::bind2nd(std::multiplies<double>(), dx));

  HalfComplex res(m_resolution.data(), nData);
  HalfComplex fun(out, nData);

  for (size_t i = 0; i <= res.size(); i++) {
    // complex multiplication
    double res_r = res.real(i);
    double res_i = res.imag(i);
    double fun_r = fun.real(i);
    double fun_i = fun.imag(i);
    fun.set(i, res_r * fun_r - res_i * fun_i, res_r * fun_i + res_i * fun_r);
  }

  gsl_fft_halfcomplex_wavetable *wavetable_r =
      gsl_fft_halfcomplex_wavetable_alloc(nData);
  gsl_fft_halfcomplex_inverse(out, 1, nData, wavetable_r, workspace);
  gsl_fft_halfcomplex_wavetable_free(wavetable_r);

  gsl_fft_real_workspace_free(workspace);

  dx = nData > 1 ? 1. / (xValues[1] - xValues[0]) : 1.;
  std::transform(out, out + nData, out,
                 std::bind2nd(std::multiplies<double>(), dx));

  if (dltF != 0.) {
    double *tmp = new double[nData];
    resolution->function1D(tmp, xValues, nData);
    std::transform(tmp, tmp + nData, tmp,
                   std::bind2nd(std::multiplies<double>(), dltF));
    std::transform(out, out + nData, tmp, out, std::plus<double>());
    delete[] tmp;
  }
}

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
    if (cf == 0) {
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

} // namespace CurveFitting
} // namespace Mantid
