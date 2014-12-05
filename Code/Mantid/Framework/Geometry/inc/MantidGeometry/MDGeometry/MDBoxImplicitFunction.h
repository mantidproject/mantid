#ifndef MANTID_MDALGORITHMS_MDBOXIMPLICITFUNCTION_H_
#define MANTID_MDALGORITHMS_MDBOXIMPLICITFUNCTION_H_
    
#include "MantidKernel/System.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/VMD.h"


namespace Mantid
{
namespace Geometry
{

  /** General N-dimensional box implicit function:
   * Defines a cuboid in N dimensions that is aligned with the axes
   * of a MDEventWorkspace.
    
    @author Janik Zikovsky
    @date 2011-07-21

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport MDBoxImplicitFunction : public MDImplicitFunction
  {
  public:
    MDBoxImplicitFunction();

    MDBoxImplicitFunction(const Mantid::Kernel::VMD & min, const Mantid::Kernel::VMD & max);

    MDBoxImplicitFunction(const std::vector<coord_t> & min, const std::vector<coord_t> & max);

    void construct(const Mantid::Kernel::VMD & min, const Mantid::Kernel::VMD & max);

    virtual ~MDBoxImplicitFunction();
    
  };


} // namespace Geometry
} // namespace Mantid

#endif  /* MANTID_MDALGORITHMS_MDBOXIMPLICITFUNCTION_H_ */
