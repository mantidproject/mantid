#ifndef MD_WORKSPACE_HELPER_H
#define MD_WORKSPACE_HELPER_H

#include "MDDataObjects/MDWorkspace.h"
/** set of classes intended to produce different MDWorkspaces for debugging and testing purposes

    @author Alex Buts, RAL ISIS
    @date 19/11/2010

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
class MDWorkspaceMockData10: public MDWorkspace
{
public:
  MDWorkspaceMockData10(unsigned int nDims=4,unsigned int nRecDims=3);

};
}
}

#endif