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
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidCurveFitting/DllConfig.h"
#include <cmath>

namespace Mantid {

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
namespace API {
class MatrixWorkspace;
}

namespace CurveFitting {
namespace Functions {
/**

A function which takes its values from a file or a workspace. The values atr
tabulated as
x,y pairs. Liear interpolation is used for points between the tabulated values.
The function
returns zero for points outside the tabulated values.

The function has two attributes: FileName and Workspace. They define a data set
to take the values from.
Setting one of the attributes clears the other.

The files can be either ascii text files or nexus files. The ascii files must
contain two column
of real numbers separated by spaces. The first column are the x-values and the
second one is for y.

If a nexus file is used its first spectrum provides the data for the function.
The same is true for
a workspace which must be a MatrixWorkspace.

The function has two parameters - a scaling factor "Scaling" and a shift factor
along the abscissas 'Shift'

@author Roman Tolchenov, Tessella plc
@date 4/09/2012
*/
class MANTID_CURVEFITTING_DLL TabulatedFunction : public API::ParamFunction, public API::IFunction1D {
public:
  /// Constructor
  TabulatedFunction();

  /// overwrite IFunction base class methods
  std::string name() const override { return "TabulatedFunction"; }
  const std::string category() const override { return "General"; }
  void function1D(double *out, const double *xValues, const size_t nData) const override;
  ///  function derivatives
  void functionDeriv1D(API::Jacobian *out, const double *xValues, const size_t nData) override;

  /// Returns the number of attributes associated with the function
  size_t nAttributes() const override;
  /// Returns a list of attribute names
  std::vector<std::string> getAttributeNames() const override;
  /// Return a value of attribute attName
  Attribute getAttribute(const std::string &attName) const override;
  /// Set a value to attribute attName
  void setAttribute(const std::string &attName, const IFunction::Attribute &value) override;

private:
  /// Call the appropriate load function
  void load(const std::string &fname);

  /// Load the points from a MatrixWorkspace
  void loadWorkspace(const std::string &wsName) const;

  /// Load the points from a MatrixWorkspace
  void loadWorkspace(std::shared_ptr<API::MatrixWorkspace> ws) const;

  /// Size of the data
  size_t size() const { return m_yData.size(); }

  /// Clear all data
  void clear() const;

  /// Evaluate the function for a list of arguments and given scaling factor
  void eval(double scaling, double xshift, double xscale, double *out, const double *xValues, const size_t nData) const;

  /// Fill in the x and y value containers (m_xData and m_yData)
  void setupData() const;

  /// The default value for the workspace index
  static const int defaultIndexValue;

  /// Temporary workspace holder
  mutable std::shared_ptr<API::MatrixWorkspace> m_workspace;

  /// Stores x-values
  mutable std::vector<double> m_xData;

  /// Stores y-values
  mutable std::vector<double> m_yData;

  /// Flag of completing data setup
  mutable bool m_setupFinished;

  /// Flag of explicit x-y data setup
  mutable bool m_explicitXY;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
