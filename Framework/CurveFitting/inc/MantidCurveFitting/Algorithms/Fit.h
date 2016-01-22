// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_FIT_H_
#define MANTID_CURVEFITTING_FIT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/IFittingAlgorithm.h"
//=====================================================
#include "MantidCurveFitting/GSLVector.h"

namespace Mantid {

namespace API {
class FunctionDomain;
class FunctionValues;
class Workspace;
class IFuncMinimizer;
} // namespace API

namespace CurveFitting {
namespace CostFunctions {
  class CostFuncFitting;
}

namespace Algorithms {
/**

A generic fitting algorithm. It fits a function to some data in a workspace.

The static properties are:
<UL>
  <LI>Function - The fitting function</LI>
  <LI>InputWorkspace - First input workspace with the data</LI>
  <LI>DomainType - The type of function domain to use: Simple, Sequential, or
Parallel</LI>
  <LI>Ties - Optional parameter ties</LI>
  <LI>Constraints - Optional parameter constraints</LI>
  <LI>MaxIterations - Max number of iterations, default 500</LI>
  <LI>OutputStatus - A string with the output status</LI>
  <LI>OutputChi2overDoF - The final chi^2 over degrees of freedom</LI>
  <LI>Minimizer - The minimizer, default Levenberg-Marquardt</LI>
  <LI>CostFunction - The cost function , default Least squares</LI>
  <LI>CreateOutput - A flag to create output workspaces.</LI>
  <LI>Output - Optional base name for the output workspaces.</LI>
</UL>

After setting "Function" and "InputWorkspace" additional dynamic properties can
be declared.
Property "Function" must be set first. If it is of a multi-domain variety the
algorithm will
declare a number of properties with names "InputWorkspace_#" where # stands for
a number from 1
to n-1 where n is the number of domains required for the function. All the
workspace properties
have to be set.

After a "InputWorkspace[_#]" property is set more dynamic proeprties can be
declared. This depends
on the functions and the type of the workspace. For example, if "Function" is
IFunction1D and
"InputWorkspace" is a MatrixWorkspace then properties "WorkspaceIndex",
"StartX", and "EndX" will
be added.

If the output workspaces are to be created they will have the following
properties:
<UL>
  <LI>OutputNormalisedCovarianceMatrix - A TableWorkspace with the covariance
matrix</LI>
  <LI>OutputParameters - A TableWorkspace with the optimized parameters</LI>
  <LI>OutputWorkspace - Optional: some functions and input workspaces may alow
to create a workspace
      with the calculated values</LI>
</UL>

@author Roman Tolchenov, Tessella plc
@date 06/12/2011
*/
class DLLExport Fit : public IFittingAlgorithm {
public:
  /// Default constructor
  Fit();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "Fit"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Fits a function to data in a Workspace";
  }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"FitGaussian", "UserFunction1D", "PlotPeakByLogValue",
            "SplineBackground", "EvaluateFunction"};
  }

private:
  void initConcrete() override;
  void execConcrete() override;
  void readProperties();
  void initializeMinimizer(size_t maxIterations);
  size_t runMinimizer();
  void finalizeMinimizer(size_t nIterations);
  void copyMinimizerOutput(const API::IFuncMinimizer &minimizer);
  void createOutput();
  /// The cost function
  boost::shared_ptr<CostFunctions::CostFuncFitting> m_costFunction;
  /// The minimizer
  boost::shared_ptr<API::IFuncMinimizer> m_minimizer;
  /// Max number of iterations
  size_t m_maxIterations;
  void outputSurface();
  void pushParameters(const CostFunctions::CostFuncFitting& fun);

  std::vector<GSLVector> m_points;
};

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_FIT_H_*/
