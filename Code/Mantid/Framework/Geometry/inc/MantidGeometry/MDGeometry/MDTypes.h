#ifndef MANTID_GEOMETRY_MDTYPES_H_
#define MANTID_GEOMETRY_MDTYPES_H_

/**
    This file contains typedefs for entities relating to multi-dimensional
    geometry.

    @author Martyn Gigg, Tessella plc

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

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

#include <limits>

namespace Mantid {

/** Typedef for the data type to use for coordinate axes in MD objects such
 * as MDBox, MDEventWorkspace, etc.
 *
 * This could be a float or a double, depending on requirements.
 * We can change this in order to compare
 * performance/memory/accuracy requirements.
 */
typedef float coord_t;

/// Define indicating that the coord_t type is a float (not double)
//#undef COORDT_IS_FLOAT
#define COORDT_IS_FLOAT

/** Typedef for the signal recorded in a MDBox, etc.
 * Note: MDEvents use 'float' internally to save memory
 */
typedef double signal_t;

/** Macro TMDE to make declaring template functions
 * faster. Put this macro before function declarations.
 * Use:
 * TMDE(void ClassName)\:\:methodName()
 * {
 *    // function body here
 * }
 *
 * @tparam MDE :: the MDLeanEvent/MDEvent type; at first, this will always be
 *                MDLeanEvent<nd>
 * @tparam nd :: the number of dimensions in the center coords. Passing this
 *               as a template argument should speed up some code.
 */
#define TMDE(decl) template <typename MDE, size_t nd> decl<MDE, nd>

/** Macro to make declaring template classes faster.
 * Use:
 * TMDE_CLASS
 * class ClassName : ...
 */
#define TMDE_CLASS template <typename MDE, size_t nd>

#define UNDEF_SIZET std::numeric_limits<size_t>::max()
#define UNDEF_COORDT std::numeric_limits<coord_t>::quiet_NaN()
#define UNDEF_UINT64 std::numeric_limits<uint64_t>::max()
}

#endif // MANTID_GEOMETRY_MDTYPES_H_
