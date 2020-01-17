// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_PLOTPEAKBULOGVALUE_H_
#define MANTID_CURVEFITTING_PLOTPEAKBULOGVALUE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidCurveFitting/Algorithms/PlotPeakByLogValueHelper.h"

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {
/**

Takes a workspace group and fits the same spectrum in all workspaces with
the same function. The output parameters are saved in a workspace ready
for plotting against the specified log value.

Required Properties:
<UL>
<LI> InputWorkspace - The name of the WorkspaceGroup to take as input </LI>
<LI> OutputWorkspace - The name of the workspace in which to store the result
</LI>
<LI> WorkspaceIndex - The index of the spectrum to fit.</LI>
<LI> Optimization - A list property defining how to set initial guesses for
      the parameters. Value 'Individual' ... </LI>
<LI> Function - The fitting function. </LI>
<LI> LogValue - The log value to plot against. </LI>
<LI> Parameters - A list (comma separated) of parameters for plotting. </LI>
</UL>


@author Roman Tolchenov, Tessella plc
@date 01/06/2010
*/
class DLLExport PlotPeakByLogValue : public API::Algorithm {
  /** Structure to identify data for fitting
   */

public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "PlotPeakByLogValue"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Fits a number of spectra with the same function.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"Fit"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Optimization"; }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;

  /// Set any WorkspaceIndex attributes in the fitting function
  void setWorkspaceIndexAttribute(API::IFunction_sptr fun, int wsIndex) const;

  boost::shared_ptr<Algorithm> runSingleFit(bool createFitOutput,
                                            bool outputCompositeMembers,
                                            bool outputConvolvedMembers,
                                            const API::IFunction_sptr &ifun,
                                            const InputSpectraToFit &data);

  double calculateLogValue(std::string logName, const InputSpectraToFit &data);

  API::ITableWorkspace_sptr
  createResultsTable(const std::string &logName,
                     const API::IFunction_sptr &ifunSingle, bool &isDataName);

  void appendTableRow(bool isDataName, API::ITableWorkspace_sptr &result,
                      const API::IFunction_sptr &ifun,
                      const InputSpectraToFit &data, double logValue,
                      double chi2) const;

  void finaliseOutputWorkspaces(
      bool createFitOutput,
      const std::vector<API::MatrixWorkspace_sptr> &fitWorkspaces,
      const std::vector<API::ITableWorkspace_sptr> &parameterWorkspaces,
      const std::vector<API::ITableWorkspace_sptr> &covarianceWorkspaces);

  API::IFunction_sptr setupFunction(bool individual, bool passWSIndexToFunction,
                                    const API::IFunction_sptr &inputFunction,
                                    const std::vector<double> &initialParams,
                                    bool isMultiDomainFunction, int i,
                                    const InputSpectraToFit &data) const;

  /// Create a minimizer string based on template string provided
  std::string getMinimizerString(const std::string &wsName,
                                 const std::string &wsIndex);

  /// Base name of output workspace
  std::string m_baseName;

  /// Record of workspaces output by the minimizer
  std::map<std::string, std::vector<std::string>> m_minimizerWorkspaces;
};

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_PLOTPEAKBULOGVALUE_H_*/
