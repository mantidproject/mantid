// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_COSTFUNCLEASTSQUARES_H_
#define MANTID_CURVEFITTING_COSTFUNCLEASTSQUARES_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/CostFunctions/CostFuncFitting.h"
#include "MantidCurveFitting/GSLMatrix.h"
#include "MantidCurveFitting/GSLVector.h"

namespace Mantid {
namespace CurveFitting {
class SeqDomain;
class ParDomain;

namespace CostFunctions {
/** Cost function for least squares

    @author Anders Markvardsen, ISIS, RAL
    @date 11/05/2010
*/
class DLLExport CostFuncLeastSquares : public CostFuncFitting {
public:
  /// Constructor
  CostFuncLeastSquares();

  /// Get name of minimizer
  std::string name() const override { return "Least squares"; }

  /// Get short name of minimizer - useful for say labels in guis
  std::string shortName() const override { return "Chi-sq"; };

  /// Calculate value of cost function
  double val() const override;

  /// Calculate the derivatives of the cost function
  /// @param der :: Container to output the derivatives
  void deriv(std::vector<double> &der) const override;

  /// Calculate the value and the derivatives of the cost function
  /// @param der :: Container to output the derivatives
  /// @return :: The value of the function
  double valAndDeriv(std::vector<double> &der) const override;

  virtual double valDerivHessian(bool evalDeriv = true,
                                 bool evalHessian = true) const;
  const GSLVector &getDeriv() const;
  const GSLMatrix &getHessian() const;
  void push();
  void pop();
  void drop();

protected:
  void calActiveCovarianceMatrix(GSLMatrix &covar,
                                 double epsrel = 1e-8) override;

  void addVal(API::FunctionDomain_sptr domain,
              API::FunctionValues_sptr values) const;
  void addValDerivHessian(API::IFunction_sptr function,
                          API::FunctionDomain_sptr domain,
                          API::FunctionValues_sptr values,
                          bool evalDeriv = true, bool evalHessian = true) const;

  /// Get mapped weights from FunctionValues
  virtual std::vector<double>
  getFitWeights(API::FunctionValues_sptr values) const;

  /// Flag to include constraint in cost function value
  bool m_includePenalty;

  mutable double m_value;
  mutable GSLVector m_der;
  mutable GSLMatrix m_hessian;

  mutable bool m_pushed;
  mutable double m_pushedValue;
  mutable GSLVector m_pushedParams;

  friend class CurveFitting::SeqDomain;
  friend class CurveFitting::ParDomain;

  double m_factor;
};

} // namespace CostFunctions
} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_COSTFUNCLEASTSQUARES_H_*/
