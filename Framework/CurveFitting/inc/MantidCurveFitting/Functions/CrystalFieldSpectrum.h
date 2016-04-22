#ifndef MANTID_CURVEFITTING_CRYSTALFIELDSPECTRUM_H_
#define MANTID_CURVEFITTING_CRYSTALFIELDSPECTRUM_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidCurveFitting/Functions/CrystalFieldPeaks.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
Calculates crystal field spectrum. 

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport CrystalFieldSpectrum : public API::IFunction1D {
public:

  /// Constructor
  CrystalFieldSpectrum();

  /// overwrite IFunction base class methods
  std::string name() const override { return "CrystalFieldSpectrum"; }
  const std::string category() const override { return "General"; }
  /// Evaluate the function
  void function1D(double *out, const double *xValues,
                          const size_t nData) const override;

  /// Set i-th parameter
  virtual void setParameter(size_t, const double &value,
                            bool explicitlySet = true) override;
  /// Set i-th parameter description
  virtual void setParameterDescription(size_t,
                                       const std::string &description) override;
  /// Get i-th parameter
  virtual double getParameter(size_t i) const override;
  /// Set parameter by name.
  virtual void setParameter(const std::string &name, const double &value,
                            bool explicitlySet = true) override;
  /// Set description of parameter by name.
  virtual void setParameterDescription(const std::string &name,
                                       const std::string &description) override;
  /// Get parameter by name.
  virtual double getParameter(const std::string &name) const override;
  /// Total number of parameters
  virtual size_t nParams() const override;
  /// Returns the index of parameter name
  virtual size_t parameterIndex(const std::string &name) const override;
  /// Returns the name of parameter i
  virtual std::string parameterName(size_t i) const override;
  /// Returns the description of parameter i
  virtual std::string parameterDescription(size_t i) const override;
  /// Checks if a parameter has been set explicitly
  virtual bool isExplicitlySet(size_t i) const override;
  /// Get the fitting error for a parameter
  virtual double getError(size_t i) const override;
  /// Set the fitting error for a parameter
  virtual void setError(size_t i, double err) override;

  /// Check if a declared parameter i is fixed
  virtual bool isFixed(size_t i) const override;
  /// Removes a declared parameter i from the list of active
  virtual void fix(size_t i) override;
  /// Restores a declared parameter i to the active status
  virtual void unfix(size_t i) override;

  void setAttribute(const std::string &attName, const Attribute &) override;

protected:
  /// overwrite IFunction base class method, which declare function parameters
  void init() override;

private:
  /// Function that calculates peak centres and intensities.
  CrystalFieldPeaks m_crystalField;
  API::CompositeFunction m_peaks;
  size_t m_nOwnParams;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_CRYSTALFIELDSPECTRUM_H_*/
