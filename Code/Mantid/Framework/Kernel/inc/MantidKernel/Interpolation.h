#ifndef MANTID_KERNEL_INTERPOLATION_H_
#define MANTID_KERNEL_INTERPOLATION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Property.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Unit.h"
#include <map>
#include <vector>
#include <sstream>
#include <stdlib.h>
#include <cctype>

namespace Mantid
{
namespace Kernel
{
/** 
 Provide interpolation over a series of points.
 
 @author Anders Markvardsen, ISIS, RAL
 @date 9/3/2010

 Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
 
 File change history is stored at: <https://github.com/mantidproject/mantid>.
 Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_KERNEL_DLL Interpolation
{
private:
  ///internal storage of x values
  std::vector<double> m_x;
  ///internal storage of y values
  std::vector<double> m_y;

  /// method used for doing the interpolation
  std::string m_method; 

  /// unit of x-axis
  Unit_sptr m_xUnit;

  /// unit of y-axis
  Unit_sptr m_yUnit;

protected:
  size_t findIndexOfNextLargerValue(const std::vector<double> &data, double key, size_t range_start, size_t range_end) const;

public:

  /// Constructor default to linear interpolation and x-unit set to TOF
  Interpolation();
  virtual ~Interpolation() { }

  /// add data point
  void addPoint(const double& xx, const double& yy);

  /// get interpolated value at location at
  double value(const double& at) const;

  /// set interpolation method
  void setMethod(const std::string& method) { m_method=method; }

  /// get interpolation method
  std::string getMethod() const { return m_method; };

  /// set x-axis unit
  void setXUnit(const std::string& unit);

  /// set y-axis unit
  void setYUnit(const std::string& unit);

  /// get x-axis unit
  Unit_sptr getXUnit() const { return m_xUnit; };

  /// get y-axis unit
  Unit_sptr getYUnit() const { return m_yUnit; };

  /// return false if no data has been added
  bool containData() const { return m_x.size() ? true : false;}

  /// Prints object to stream
  void printSelf(std::ostream& os) const;

  /// Clear interpolation values
  void resetData();
};

// defining operator << and >>
MANTID_KERNEL_DLL std::ostream& operator<<(std::ostream&, const Interpolation& );
MANTID_KERNEL_DLL std::istream& operator>>(std::istream&, Interpolation&);

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_INTERPOLATION_H_*/
