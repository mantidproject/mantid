// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/IBackgroundFunction.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid {
namespace Algorithms {

/** GeneratePeaks : Generate peaks from a table workspace containing peak
  parameters

  @date 2012-04-10
*/
class MANTID_ALGORITHMS_DLL GeneratePeaks final : public API::Algorithm {
public:
  GeneratePeaks();

  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "GeneratePeaks"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Generate peaks in an output workspace according to a "
           "TableWorkspace containing a list of peak's parameters.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"FindPeaks", "MatchPeaks"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Crystal\\Peaks"; }

private:
  void init() override;

  /// Implement abstract Algorithm methods
  void exec() override;

  /// Process algorithm properties
  void processAlgProperties(std::string &peakfunctype, std::string &bkgdfunctype);

  /// Process column names with peak parameter names
  void processTableColumnNames();

  void importPeaksFromTable(std::map<specnum_t, std::vector<std::pair<double, API::IFunction_sptr>>> &functionmap);

  /// Import peak and background function parameters from vector
  void importPeakFromVector(std::vector<std::pair<double, API::IFunction_sptr>> &functionmap);

  /// Generate peaks in output data workspaces
  void generatePeaks(const std::map<specnum_t, std::vector<std::pair<double, API::IFunction_sptr>>> &functionmap,
                     const API::MatrixWorkspace_sptr &dataWS);

  /// Check whether function has a certain parameter
  bool hasParameter(const API::IFunction_sptr &function, const std::string &paramname);

  /// Create output workspace
  API::MatrixWorkspace_sptr createOutputWorkspace();

  API::MatrixWorkspace_sptr createDataWorkspace(const std::vector<double> &binparameters) const;

  void createFunction(std::string &peaktype, std::string &bkgdtype);

  void getSpectraSet(const DataObjects::TableWorkspace_const_sptr &peakParmsWS);

  /// Get the IPeakFunction part in the input function
  API::IPeakFunction_sptr getPeakFunction(const API::IFunction_sptr &infunction);

  /// Add function parameter names to
  std::vector<std::string> addFunctionParameterNames(const std::vector<std::string> &funcnames);

  /// Peak function
  API::IPeakFunction_sptr m_peakFunction;

  /// Background function
  API::IBackgroundFunction_sptr m_bkgdFunction;

  ///
  std::vector<double> m_vecPeakParamValues;
  ///
  std::vector<double> m_vecBkgdParamValues;

  /// Spectrum map from full spectra workspace to partial spectra workspace
  std::map<specnum_t, specnum_t> m_SpectrumMap;

  /// Set of spectra (workspace indexes) in the original workspace that contain
  /// peaks to generate
  std::set<specnum_t> m_spectraSet;

  /// Flag to use automatic background (???)
  bool m_useAutoBkgd;

  /// Parameter table workspace
  DataObjects::TableWorkspace_sptr m_funcParamWS;

  /// Input workspace (optional)
  API::MatrixWorkspace_const_sptr inputWS;

  /// Flag whether the new workspace is exactly as input
  bool m_newWSFromParent;

  /// Binning parameters
  std::vector<double> binParameters;

  /// Flag to generate background
  bool m_genBackground;

  /// Flag to indicate parameter table workspace containing raw parameters names
  bool m_useRawParameter;

  /// Maximum chi-square to have peak generated
  double m_maxChi2;

  /// Number of FWHM for peak to extend
  double m_numPeakWidth;

  /// List of functions' parameters naems
  std::vector<std::string> m_funcParameterNames;

  /// Indexes of height, centre, width, a0, a1, and a2 in input parameter table
  int i_height, i_centre, i_width, i_a0, i_a1, i_a2;

  /// Flag to use parameter table workspace
  bool m_useFuncParamWS;

  /// Spectrum if only 1 peak is to plot
  int m_wsIndex;
};

} // namespace Algorithms
} // namespace Mantid
