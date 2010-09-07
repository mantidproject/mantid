#ifndef OPENCASCADE_CONFIG_H_
#define OPENCASCADE_CONFIG_H_

/** 
  Include this header before any OpenCascade headers to suppress warnings about macro redefinitions

  @author Martyn Gigg, Tessella plc
  @date 06/09/2010

  Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

#ifndef __MATH_WNT_H
  #undef M_SQRT1_2
  #undef M_PI_2
#endif
//Open Cascade maths header
#include <Standard_math.hxx>

#endif //OPENCASCADE_CONFIG_H_