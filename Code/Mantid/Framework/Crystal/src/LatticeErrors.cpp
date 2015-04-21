/*
 * LatticeErrors.cpp
 *

 */
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/IPeak.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidCrystal/LatticeErrors.h"
#include "MantidCrystal/SCDPanelErrors.h"
#include "MantidCrystal/OptimizeLatticeForCellType.h"

using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using Mantid::Geometry::CompAssembly;
using Mantid::Geometry::IObjComponent_const_sptr;

namespace Mantid {

namespace Crystal {
namespace {
/// static logger
Kernel::Logger g_log("LatticeErrors");
}

DECLARE_FUNCTION(LatticeErrors)

LatticeErrors::LatticeErrors() : ParamFunction(), IFunction1D() {}

LatticeErrors::~LatticeErrors() {}

void LatticeErrors::init() {
  declareParameter("p0", 0.0, "a lattice param");
  declareParameter("p1", 0.0, "b lattice param");
  declareParameter("p2", 0.0, "c lattice param");
  declareParameter("p3", 0.0, "alpha lattice param");
  declareParameter("p4", 0.0, "beta lattice param");
  declareParameter("p5", 0.0, "gamma lattice param");
}

/**
 * Calculates the h,k, and l offsets from an integer for (some of )the peaks,
 *given the parameter values.
 *
 * @param out  For each peak element in this array.
 * @param xValues  xValues give the index in the PeaksWorkspace for the peak.
 *For each peak considered there are
 *              three consecutive entries all with the same index
 * @param nData The size of the xValues and out arrays
 */
void LatticeErrors::function1D(double *out, const double *xValues,
                               const size_t nData) const {
  UNUSED_ARG(xValues);
  std::string PeakWorkspaceName = "_peaks";
  std::vector<double> Params;
  Params.push_back(getParameter("p0"));
  Params.push_back(getParameter("p1"));
  Params.push_back(getParameter("p2"));
  Params.push_back(getParameter("p3"));
  Params.push_back(getParameter("p4"));
  Params.push_back(getParameter("p5"));

  Mantid::Crystal::OptimizeLatticeForCellType u;
  u.optLattice(PeakWorkspaceName, Params, out);
  double ChiSqTot = 0;
  for (size_t i = 0; i < nData; i++) {
    ChiSqTot += out[i];
  }

  g_log.debug() << "    Chi**2 = " << ChiSqTot << "     nData = " << nData
                << std::endl;
}
void LatticeErrors::functionDerivLocal(API::Jacobian *, const double *,
                                       const size_t) {
  throw Mantid::Kernel::Exception::NotImplementedError(
      "functionDerivLocal is not implemented for LatticeErrors.");
}

void LatticeErrors::functionDeriv(const API::FunctionDomain &domain,
                                  API::Jacobian &jacobian) {
  calNumericalDeriv(domain, jacobian);
}
}
}
