#ifndef _MD_GEOMETRY_BAISIS_DIMENSION_H
#define _MD_GEOMETRY_BAISIS_DIMENSION_H

/** Represents a 'basis' dimension. Basis dimensions are those known from disk, that are in their raw unbinned form.
*   

    @author Owen Arnold, RAL ISIS
    @date 25/11/2010

    Copyright &copy; 2007-10 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

#include "MantidKernel/System.h"

namespace Mantid
{
  namespace Geometry
  {

    class DLLExport MDBasisDimension
    {
    public:
      explicit MDBasisDimension(std::string id, bool isReciprocal, int columnNumber);
      
      bool operator==(const MDBasisDimension &other) const;
      bool operator!=(const MDBasisDimension &other) const;
      bool operator < (const MDBasisDimension &other) const;

      std::string getId() const;
      bool getIsReciprocal() const;
      int getColumnNumber() const;

    private:
      std::string m_id; //Equivalent to tag in older definitions.
      bool m_isReciprocal;
      int m_columnNumber; //Column number correponding to the particular tag.
      //TODO: Add some convertable Unit ie. energy
    };

  }
}

#endif
