#ifndef H_DIMENSION_IDS
#define H_DIMENSION_IDS
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
//#include "MantidAPI/MatrixWorkspace.h"
#include "MDDataObjects/stdafx.h"
/** The class is the part of the VisualisationWorkspace and provides a default name, Identifuer and numerator for a dimension
*   set of dimensions forms workspace geometry and some dimensions have to be treated differently from others
*
TO DO: not implemented yet;

    @author Alex Buts, RAL ISIS
    @date 27/09/2010

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


namespace Mantid
{
namespace MDDataObjects
{

class DimensionIDS
{
public:
    DimensionIDS(void);
    ~DimensionIDS(void);
};
}
}
#endif

