#ifndef STD_OPERATOR_DEFINES_H_
#define STD_OPERATOR_DEFINES_H_

#include <MantidKernel/InstrumentInfo.h>

/** 
    Some classes that are exported to Python do not define certain operators that are necessary
    to wrap them. These operators are added here.

    @author Martyn Gigg, Tessella plc
    @date 12/08/2010

    Copyright &copy; 2010 STFC Rutherford Appleton Laboratory

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
namespace std
{
  /**
   * Output an InstrumentInfo object to a stream
   * @param os The output stream.
   * @param instr The object to write out.
   * @returns A reference to the stream that was written to.
   */
  inline ostream & operator<<(ostream & os, const Mantid::Kernel::InstrumentInfo& instr)
  {
    os << instr.name();
    return os;
  }
}


#endif //STD_OPERATOR_DEFINES_H_
