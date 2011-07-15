#ifndef MANTID_MDALGORITHMS_MDIMPLICITFUNCTION_H_
#define MANTID_MDALGORITHMS_MDIMPLICITFUNCTION_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/ImplicitFunction.h"


namespace Mantid
{
namespace MDAlgorithms
{

  /** An "ImplicitFunction" defining a hyper-cuboid-shaped region in N dimensions.
   * This is to be used in various MD rebinning algorithms to determine
   * e.g, which boxes should be considered to be within the integration volume.
   *
   * This general case would cover boxes that are not aligned with the axes.
   *
   * Various shapes can be built by intersecting 1 or more planes.
   * The Plane, and whether a point is bounded by it, will be the basis
   * of determining whether a point is in a volume.
   *
   * For example, in a 3D space:
   *
   * 1 plane = a half-infinite volume
   * 2 parallel planes = a plane with a thickness
   * 4 aligned planes = an infinite line, rectangular in cross-section
   * 6 planes = a cuboid
   *
   * For most efficiency, each MDImplicitFunction should be built with
   * a given set of dimensions in mind; that is, if it is to be applied on
   * a MDEventWorkspace with say 6 dimensions: X, Y, Z, time, temperature, field;
   * then a mask that only looks at the relevant 3 dimensions is used.
    
    @author Janik Zikovsky
    @date 2011-07-15

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport MDImplicitFunction // : public Mantid::API::ImplicitFunction
  {
  public:
    MDImplicitFunction();
    ~MDImplicitFunction();
    
  };


} // namespace MDAlgorithms
} // namespace Mantid

#endif  /* MANTID_MDALGORITHMS_MDIMPLICITFUNCTION_H_ */
