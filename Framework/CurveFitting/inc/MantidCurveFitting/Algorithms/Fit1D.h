// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IFunction.h"
#include "MantidCurveFitting/DllConfig.h"

namespace Mantid {

namespace CurveFitting {
namespace Algorithms {
/**
Deprecation notice: instead of using this algorithm please use the Fit algorithm
instead.

Abstract base class for 1D fitting functions.

Properties common for all fitting functions:
<UL>
<LI> InputWorkspace - The name of the Workspace2D to take as input </LI>

<LI> SpectrumNumber - The spectrum to fit, using the workspace numbering of the
spectra (default 0)</LI>
<LI> StartX - Lowest value of x data array </LI>
<LI> EndX - Highest value of x data array </LI>

<LI> Properties defined in derived class goes here

<LI> MaxIterations - The spectrum to fit (default 500)</LI>
<LI> OutputStatus - whether the fit was successful. Direction::Output</LI>
<LI> OutputChi2overDoF - returns how good the fit was (default 0.0).
Direction::Output</LI>
</UL>

@author Anders Markvardsen, ISIS, RAL
@date 15/5/2009
*/
class MANTID_CURVEFITTING_DLL Fit1D : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "Fit1D"; }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return (1); }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Optimization"; }

  /// Function you want to fit to.
  virtual void function(const double *in, double *out, const double *xValues, const size_t nData) = 0;
  /// Derivatives of function with respect to parameters you are trying to fit
  virtual void functionDeriv(const double *in, API::Jacobian *out, const double *xValues, const size_t nData);

protected:
  ~Fit1D() override = default;
  // Overridden Algorithm methods
  void init() override;
  void exec() override;

  /// Option for providing intelligent range starting value based e.g. on the
  /// user input parameter values
  virtual void modifyStartOfRange(double &startX) {
    (void)startX; // Avoid compiler warning
  }
  /// Option for providing intelligent range finishing value based e.g. on the
  /// user input parameter values
  virtual void modifyEndOfRange(double &endX) {
    (void)endX; // Avoid compiler warning
  }

  /// Declare additional properties other than fitting parameters
  virtual void declareAdditionalProperties() {};
  /// Called in the beginning of exec(). Custom initialization
  virtual void prepare() {};

  /** Called after the data ranged has been determined but before the fitting
   *starts.
   *  For example may be used to create wavelength array for each TOF
   *data-point.
   *  Number of data point to fit over are m_maxX-m_minX.
   *
   *  @param m_minX :: Start array index.
   *  @param m_maxX :: End array index.
   */
  virtual void afterDataRangedDetermined(const int &m_minX, const int &m_maxX) {
    (void)m_minX;
    (void)m_maxX; // Avoid compiler warning
  };

  /// Declare parameters specific to fitting function
  virtual void declareParameters() = 0;

  /// Overload this function if the actual fitted parameters are different from
  /// those the user specifies.
  virtual void modifyInitialFittedParameters(std::vector<double> &fittedParameter);

  /// If modifyInitialFittedParameters is overloaded this method must also be
  /// overloaded
  /// to reverse the effect of modifyInitialFittedParameters before outputting
  /// the results back to the user
  virtual void modifyFinalFittedParameters(std::vector<double> &fittedParameter);

  /// Holds a copy of the value of the parameters that are actually
  /// least-squared fitted.
  std::vector<double> m_fittedParameter;

  /// Holds a copy of the names of the fitting parameters
  std::vector<std::string> m_parameterNames;

  /// Number of parameters (incuding fixed).
  size_t nParams() const { return m_parameterNames.size(); }

  friend struct FitData;
};

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
