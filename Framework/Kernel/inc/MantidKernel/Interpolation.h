// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_INTERPOLATION_H_
#define MANTID_KERNEL_INTERPOLATION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/Unit.h"
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

namespace Mantid {
namespace Kernel {
/**
 Provide interpolation over a series of points.

 @author Anders Markvardsen, ISIS, RAL
 @date 9/3/2010
*/
class MANTID_KERNEL_DLL Interpolation {
private:
  /// internal storage of x values
  std::vector<double> m_x;
  /// internal storage of y values
  std::vector<double> m_y;

  /// method used for doing the interpolation
  std::string m_method;

  /// unit of x-axis
  Unit_sptr m_xUnit;

  /// unit of y-axis
  Unit_sptr m_yUnit;

protected:
  size_t findIndexOfNextLargerValue(const std::vector<double> &data,
                                    double key) const;

public:
  /// Constructor default to linear interpolation and x-unit set to TOF
  Interpolation();
  virtual ~Interpolation() = default;

  /// add data point
  void addPoint(const double &xx, const double &yy);

  /// get interpolated value at location at
  double value(const double &at) const;

  /// set interpolation method
  void setMethod(const std::string &method) { m_method = method; }

  /// get interpolation method
  std::string getMethod() const { return m_method; };

  /// set x-axis unit
  void setXUnit(const std::string &unit);

  /// set y-axis unit
  void setYUnit(const std::string &unit);

  /// get x-axis unit
  Unit_sptr getXUnit() const { return m_xUnit; };

  /// get y-axis unit
  Unit_sptr getYUnit() const { return m_yUnit; };

  /// return false if no data has been added
  bool containData() const { return !m_x.empty() ? true : false; }

  /// Prints object to stream
  void printSelf(std::ostream &os) const;

  /// Clear interpolation values
  void resetData();
};

// defining operator << and >>
MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream &,
                                           const Interpolation &);
MANTID_KERNEL_DLL std::istream &operator>>(std::istream &, Interpolation &);

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_INTERPOLATION_H_*/
