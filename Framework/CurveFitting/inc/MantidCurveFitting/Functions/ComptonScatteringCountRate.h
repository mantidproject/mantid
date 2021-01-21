// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/CompositeFunction.h"
#include "MantidCurveFitting/Functions/ComptonProfile.h"
#include "MantidKernel/Matrix.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/**
  Implements a specialized function that encapsulates the combination of
  ComptonProfile functions
  that give the Neutron count rate.
*/
class MANTID_CURVEFITTING_DLL ComptonScatteringCountRate : public API::CompositeFunction {
public:
  /// Constructor
  ComptonScatteringCountRate();

private:
  /// String identifier
  std::string name() const override { return "ComptonScatteringCountRate"; }
  /// Set an attribute value (and possibly cache its value)
  void setAttribute(const std::string &name, const Attribute &value) override;
  /// Takes the string & constructs the constraint matrix
  void parseIntensityConstraintMatrix(const std::string &value);

  /// Called by the framework just before an iteration is starting
  void iterationStarting() override;
  /// Set the fixed parameters to the given values
  void setFixedParameterValues(const std::vector<double> &values);
  /// Refresh the values of the C matrix for this evaluation
  void updateCMatrixValues() const;

  /// Cache reference to workspace for use in setupForFit
  void setMatrixWorkspace(std::shared_ptr<const API::MatrixWorkspace> matrix, size_t wsIndex, double startX,
                          double endX) override;
  /// Cache ptrs to the individual profiles and their parameters
  void cacheFunctions();
  /// Cache ptr to the individual profile and its parameters
  void cacheComptonProfile(const std::shared_ptr<ComptonProfile> &profile, const size_t paramsOffset);
  /// Cache parameters positions for background function
  void cacheBackground(const API::IFunction1D_sptr &function1D, const size_t paramsOffset);
  /// Set up the constraint matrices
  void createConstraintMatrices();
  /// Set up positivity constraint matrix
  void createPositivityCM();
  /// Set up equality constraint matrix
  void createEqualityCM(const size_t nmasses);

  /// Holder for non-owning functions cast as ComptonProfiles
  std::vector<ComptonProfile *> m_profiles;
  /// Store parameter indices of intensity parameters that are fixed
  std::vector<size_t> m_fixedParamIndices;
  /// Positivity constraints on J(y)
  mutable Kernel::DblMatrix m_cmatrix;
  /// Intensity equality constraints
  Kernel::DblMatrix m_eqMatrix;
  /// Name of order attribute on background function
  std::string m_bkgdOrderAttr;
  /// The order of the background
  int m_bkgdPolyN;
  /// The histogram of the matrix workspace being cached for use
  std::shared_ptr<HistogramData::Histogram> m_hist;
  /// The workspace index being worked on
  size_t wsIndex;
  /// Ratio of data & errors
  std::vector<double> m_dataErrorRatio;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
