//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/Convolution.h"
#include "MantidCurveFitting/Functions/DeltaFunction.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/FunctionDomain1D.h"

#include <cmath>
#include <algorithm>
#include <functional>

#include <gsl/gsl_errno.h>
#include <gsl/gsl_fft_real.h>
#include <gsl/gsl_fft_halfcomplex.h>

#include <sstream>
#include <iostream>
#include <fstream>

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

  const size_t mData = domain.size();
  const double *xOrigVals = d1d->getPointerAt(0);
  double dx = (xOrigVals[mData-1]-xOrigVals[0])/static_cast<double>((mData-1));
  auto ixP = static_cast<size_t>(xOrigVals[mData-1]/dx); //positive x-values
  auto ixN = mData-ixP-1; // negative x-values (ixP+ixN=mData-1)

  refreshResolution();

  // double the domain where to evaluate the convolution. Guarantees complete
  // overlap betwen convolution and signal in the original range.
  const size_t nData = mData+ixN+ixP; // equal to 2*mData-1
  std::vector<double>xValuesVec(nData);
  double *xValues = xValuesVec.data();
  Mantid::API::FunctionDomain1DView domainExtd(xValues, nData);
  double Dx = dx*static_cast<double>(ixN+ixP);
  for(size_t i=0; i<nData; i++){
    xValues[i] = -Dx + static_cast<double>(i)*dx;
  }

  // Fill m_resolution with the resolution function data
  IFunction1D_sptr resolution =
      boost::dynamic_pointer_cast<IFunction1D>(getFunction(0));
  if(!resolution) {
    throw std::runtime_error("Convolution can work only with IFunction1D");
  }
  if (m_resolution.empty()) {
    m_resolution.resize(mData);
  }
  resolution->function1D(m_resolution.data(), xOrigVals, mData);

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
    //Convolve with resolution
    double *outExt = valuesExtd.getPointerToCalculated(0);
    for(size_t i=0; i<mData; i++) {
      double tmp{0.0};
      for(size_t j=0; j<mData; j++) {
        tmp += outExt[i+j]*m_resolution[j];
      }
      out[i] = tmp*dx;
    }
  }
  else {
    values.zeroCalculated();
  }

  if (dltF != 0.0 && !deltaShifted) {
    // If model contains any delta functions their effect is addition of scaled
    // resolution
    std::vector<double> tmp(mData);
    resolution->function1D(tmp.data(), xOrigVals, mData);
    std::transform(tmp.begin(), tmp.end(), tmp.begin(),
                   std::bind2nd(std::multiplies<double>(), dltF));
    std::transform(out, out + mData, tmp.data(), out, std::plus<double>());
  } else if (!dltFuns.empty()) {
    std::vector<double> x(mData);
    for (const auto &df : dltFuns) {
      double shift = -df->getParameter("Centre");
      dltF = df->getParameter("Height") * df->HeightPrefactor();
      std::transform(xOrigVals, xOrigVals + mData, x.data(),
                     std::bind2nd(std::plus<double>(), shift));
      std::vector<double> tmp(mData);
      resolution->function1D(tmp.data(), x.data(), mData);
      std::transform(tmp.begin(), tmp.end(), tmp.begin(),
                     std::bind2nd(std::multiplies<double>(), dltF));
      std::transform(out, out + mData, tmp.data(), out, std::plus<double>());
    }
  }

} // end of Convolution::function()

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
