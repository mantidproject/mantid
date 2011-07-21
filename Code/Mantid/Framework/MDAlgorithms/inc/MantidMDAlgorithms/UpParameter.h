#ifndef MANTID_MDALGORITHMS_UPPARAMETER_H_
#define MANTID_MDALGORITHMS_UPPARAMETER_H_

#include "MantidMDAlgorithms/Vector3DParameter.h"

namespace Mantid
{
namespace MDAlgorithms
{

/**
Defines a spatial vector perpendicular to a normal vector, in the 'up' direction.

@author Owen Arnold, Tessella plc
@date 01/02/2011

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

File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

  DECLARE_3D_VECTOR_PARAMETER(UpParameter, double);

}
}

#endif
