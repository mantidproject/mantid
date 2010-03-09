#ifndef MANTID_KERNEL_INTERPOLATION_H_
#define MANTID_KERNEL_INTERPOLATION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Property.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/DateAndTime.h"
#include <iostream>
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

 Copyright &copy; 2007-2010 STFC Rutherford Appleton Laboratory

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
 
 File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
 Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport Interpolation
{
private:
  std::vector<double> m_x;
  std::vector<double> m_y;

  /// method used for doing the interpolation
  std::string m_name; 

public:

  /// Constructor
  Interpolation(const std::string &name);

  /// add data point
  void addPoint(const double& xx, const double& yy);

  /// get interpolated value at location at
  double value (const double& at);


  /// static reference to the logger class
  static Logger& g_log;
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_INTERPOLATION_H_*/
