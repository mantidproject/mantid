// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"

#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

#include "MantidCurveFitting/Algorithms/LeBailFit.h"
#include "MantidCurveFitting/Constraints/BoundaryConstraint.h"
#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/Functions/ThermalNeutronDtoTOFFunction.h"

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {

/** RefinePowderInstrumentParameters3 :
 */
class MANTID_CURVEFITTING_DLL RefinePowderInstrumentParameters3 final : public API::Algorithm {
public:
  RefinePowderInstrumentParameters3();

  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "RefinePowderInstrumentParameters"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Parameters include Dtt1, Dtt1t, Dtt2t, Zero, Zerot. "; }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 3; }
  const std::vector<std::string> seeAlso() const override { return {"RefinePowderDiffProfileSeq"}; }

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Diffraction\\Fitting"; }

private:
  /// Implement abstract Algorithm methods
  void init() override;

  /// Implement abstract Algorithm methods
  void exec() override;

  /// Fit instrument parameters by non Monte Carlo algorithm
  double execFitParametersNonMC();

  /// Refine instrument parameters by Monte Carlo/simulated annealing method
  double execFitParametersMC();

  /// Do MC/simulated annealing to refine parameters
  double doSimulatedAnnealing(std::map<std::string, Parameter> inparammap);

  /// Set up Monte Carlo random walk strategy
  void setupRandomWalkStrategy(std::map<std::string, Parameter> &parammap,
                               std::vector<std::vector<std::string>> &mcgroups);

  /// Add parameter (to a vector of string/name) for MC random walk
  void addParameterToMCMinimize(std::vector<std::string> &parnamesforMC, const std::string &parname,
                                std::map<std::string, Parameter> parammap);

  /// Propose new parameters
  void proposeNewValues(const std::vector<std::string> &mcgroup, std::map<std::string, Parameter> &curparammap,
                        std::map<std::string, Parameter> &newparammap, double currchisq);

  /// Determine whether the proposed value should be accepted or denied
  bool acceptOrDenyChange(double curchisq, double newchisq, double temperature);

  /// Book keep the best fitting result
  void bookKeepMCResult(std::map<std::string, Parameter> parammap, double chisq, int istep, int igroup,
                        std::map<std::string, Parameter> &bestparammap);
  // vector<pair<double, map<string, Parameter> > > &bestresults, size_t
  // maxnumresults);

  /// Implement parameter values, calculate function and its chi square.
  double calculateFunction(const std::map<std::string, Parameter> &parammap, std::vector<double> &vecY);

  /// Calculate Chi^2 of the a function with all parameters are fixed
  double calculateFunctionError(const API::IFunction_sptr &function, const DataObjects::Workspace2D_sptr &dataws,
                                int wsindex);

  /// Fit function by non MC minimzer(s)
  double fitFunction(const API::IFunction_sptr &function, const DataObjects::Workspace2D_sptr &dataws, int wsindex,
                     bool powerfit);

  /// Fit function (single step)
  bool doFitFunction(const API::IFunction_sptr &function, const DataObjects::Workspace2D_sptr &dataws, int wsindex,
                     const std::string &minimizer, int numiters, double &chi2, std::string &fitstatus);

  /// Process input properties
  void processInputProperties();

  /// Parse TableWorkspaces
  void parseTableWorkspaces();

  /// Parse table workspace to a map of Parameters
  void parseTableWorkspace(const DataObjects::TableWorkspace_sptr &tablews, std::map<std::string, Parameter> &parammap);

  /// Set parameter values to function from Parameter map
  void setFunctionParameterValues(const API::IFunction_sptr &function, std::map<std::string, Parameter> params);

  /// Update parameter values to Parameter map from fuction map
  void updateFunctionParameterValues(API::IFunction_sptr function, std::map<std::string, Parameter> &params);

  /// Set parameter fitting setup (boundary, fix or unfix) to function from
  /// Parameter map
  void setFunctionParameterFitSetups(const API::IFunction_sptr &function, std::map<std::string, Parameter> params);

  /// Construct output
  DataObjects::Workspace2D_sptr genOutputWorkspace(const API::FunctionDomain1DVector &domain,
                                                   const API::FunctionValues &rawvalues);

  /// Construct an output TableWorkspace for refined peak profile parameters
  DataObjects::TableWorkspace_sptr genOutputProfileTable(std::map<std::string, Parameter> parameters, double startchi2,
                                                         double finalchi2);

  /// Add a parameter to parameter map.  If this parametere does exist, then
  /// replace the value of it
  void addOrReplace(std::map<std::string, Parameter> &parameters, const std::string &parname, double parvalue);

  //--------  Variables ------------------------------------------------------
  /// Data workspace containg peak positions
  DataObjects::Workspace2D_sptr m_dataWS;

  /// Workspace index of the peak positions
  int m_wsIndex;

  /// TableWorkspace containing peak parameters value and fit information
  DataObjects::TableWorkspace_sptr m_paramTable;

  /// Fit mode
  enum { FIT, MONTECARLO } m_fitMode;

  /// Standard error mode
  enum { CONSTANT, USEINPUT } m_stdMode;

  /// Monte Carlo random walk steps
  int m_numWalkSteps;

  /// Random seed
  int m_randomSeed;

  /// Data structure (map of Parameters) to hold parameters
  std::map<std::string, Parameter> m_profileParameters;

  /// My function for peak positions
  Functions::ThermalNeutronDtoTOFFunction_sptr m_positionFunc;

  /// Damping factor
  double m_dampingFactor;

  /// Book keep for MC
  double m_bestChiSq;
  int m_bestChiSqStep;
  int m_bestChiSqGroup;
};

//================================= External Functions
//=========================================
/// Convert a vector to a lookup map (dictionary)
void convertToDict(std::vector<std::string> strvec, std::map<std::string, size_t> &lookupdict);

/// Get the index from lookup dictionary (map)
int getStringIndex(std::map<std::string, size_t> lookupdict, const std::string &key);

/// Store function parameter values to a map
void storeFunctionParameterValue(const API::IFunction_sptr &function,
                                 std::map<std::string, std::pair<double, double>> &parvaluemap);

/// Restore function parameter values to a map
void restoreFunctionParameterValue(std::map<std::string, std::pair<double, double>> parvaluemap,
                                   const API::IFunction_sptr &function, std::map<std::string, Parameter> &parammap);

/// Copy parameters from source to target
void duplicateParameters(std::map<std::string, Parameter> source, std::map<std::string, Parameter> &target);

/// Copy parameters values from source to target
void copyParametersValues(std::map<std::string, Parameter> &source, std::map<std::string, Parameter> &target);

/// Calculate Chi^2
double calculateFunctionChiSquare(const std::vector<double> &modelY, const std::vector<double> &dataY,
                                  const std::vector<double> &dataE);

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
