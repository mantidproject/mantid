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
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/Unit.h"
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

namespace Mantid {
namespace Kernel {

using DataXY = std::pair<double, double>;
/**
 Provide interpolation over a series of points.

 @author Anders Markvardsen, ISIS, RAL
 @date 9/3/2010
*/
class MANTID_KERNEL_DLL Interpolation {
private:
  /// internal storage of x and y values
  std::vector<DataXY> m_data;

  /// method used for doing the interpolation
  std::string m_method;

  /// unit of x-axis
  Unit_sptr m_xUnit;

  /// unit of y-axis
  Unit_sptr m_yUnit;

protected:
  std::vector<DataXY>::const_iterator findIndexOfNextLargerValue(double key) const;
  std::vector<DataXY>::const_iterator cbegin() const;
  std::vector<DataXY>::const_iterator cend() const;

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
  const std::string &getMethod() const { return m_method; };

  /// set x-axis unit
  void setXUnit(const std::string &unit);

  /// set y-axis unit
  void setYUnit(const std::string &unit);

  /// get x-axis unit
  Unit_sptr getXUnit() const { return m_xUnit; };

  /// get y-axis unit
  Unit_sptr getYUnit() const { return m_yUnit; };

  /// return false if no data has been added
  bool containData() const { return !m_data.empty(); }

  /// Prints object to stream
  void printSelf(std::ostream &os) const;

  /// Clear interpolation values
  void resetData();
};

// defining operator << and >>
MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream &, const Interpolation &);
MANTID_KERNEL_DLL std::istream &operator>>(std::istream &, Interpolation &);

} // namespace Kernel
} // namespace Mantid
