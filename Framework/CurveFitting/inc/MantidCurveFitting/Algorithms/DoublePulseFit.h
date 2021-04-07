// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidCurveFitting/Functions/Convolution.h"
#include "MantidCurveFitting/IFittingAlgorithm.h"

namespace Mantid {

namespace API {
class FunctionDomain;
class FunctionValues;
class Workspace;
class IFuncMinimizer;
} // namespace API

namespace CurveFitting {
namespace Algorithms {
MANTID_CURVEFITTING_DLL Mantid::API::IFunction_sptr
getDoublePulseFunction(std::shared_ptr<const API::IFunction> const &function, double offset, double firstPulseWeight,
                       double secondPulseWeight);

MANTID_CURVEFITTING_DLL Mantid::API::IFunction_sptr
getDoublePulseMultiDomainFunction(std::shared_ptr<const API::MultiDomainFunction> const &function, double offset,
                                  double firstPulseWeight, double secondPulseWeight);

MANTID_CURVEFITTING_DLL Mantid::API::IFunction_sptr
extractInnerFunction(std::shared_ptr<const Mantid::CurveFitting::Functions::Convolution> const &function);

MANTID_CURVEFITTING_DLL Mantid::API::IFunction_sptr
extractInnerFunction(std::shared_ptr<const API::MultiDomainFunction> const &function);
/**

A function to fit muon data from a double pulse source. It does this by
convoluting the input fitting function with two delta functions. This has the
same interface as Fit and be used in it's place.

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
  <LI>TimeOffset - The time offset between the two pulses.</LI>
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
*/
class MANTID_CURVEFITTING_DLL DoublePulseFit : public IFittingAlgorithm {
public:
  /// Default constructor
  DoublePulseFit();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "DoublePulseFit"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "A function to fit muon data from a double pulse source. Wraps Fit.";
  }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"FitGaussian", "UserFunction1D", "PlotPeakByLogValue", "SplineBackground", "EvaluateFunction", "Fit"};
  }

private:
  void initConcrete() override;
  void execConcrete() override;

  std::vector<Mantid::API::MatrixWorkspace_sptr> getWorkspaces() const;
  void declareAdditionalProperties();
  void runFitAlgorith(Mantid::API::IAlgorithm_sptr fitAlgorithm, Mantid::API::IFunction_sptr function,
                      int maxIterations);
  void setOutputProperties();
  void createOutput(Mantid::API::IAlgorithm_sptr fitAlg, Mantid::API::IFunction_sptr function);

  bool m_makeOutput;
  bool m_outputFitData;
  bool m_multiDomain;
};

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
