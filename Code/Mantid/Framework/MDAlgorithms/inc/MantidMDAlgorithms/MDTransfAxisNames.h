#ifndef MANTID_MDALGORITHMS_MDTRANSF_AXIS_NAMES_H
#define MANTID_MDALGORITHMS_MDTRANSF_AXIS_NAMES_H
//#include "MantidDataObjects/MDTransfDEHelper.h"
#include "MantidKernel/DeltaEMode.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace MDAlgorithms {
/** The class defines default axis names/dimension ID-s for MD workspace which
  can be obtained/used
  * in the MD transformations converting matrix workspace to Q or Q-dE space
  *
  * @date 29-05-2012

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

        File/ change history is stored at:
  <https://github.com/mantidproject/mantid>
        Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

/** describes default dimensions ID currently used by multidimensional workspace
 *
 *  DimensionID is the short name which used to retrieve this dimesnion from MD
 *workspace.
 *  The names themself are defined in constructor  */
namespace CnvrtToMD {
enum defaultDimID {
  ModQ_ID,   //< the defauld |Q| id for mod Q or powder mode
  Q1_ID,     //< 1 of 3 dimID in Q3D mode
  Q2_ID,     //< 2 of 3 dimID in Q3D mode
  Q3_ID,     //< 3 of 3 dimID in Q3D mode
  dE_ID,     //< energy transfer ID
  nDefaultID //< ID conunter
};
}

class DLLExport MDTransfAxisNames {
public:
  /// function returns default dimension id-s for different Q and dE modes,
  /// defined by this class
  std::vector<std::string>
  getDefaultDimIDQ3D(Kernel::DeltaEMode::Type dEmode) const;
  std::vector<std::string>
  getDefaultDimIDModQ(Kernel::DeltaEMode::Type dEmode) const;

  // constructor
  MDTransfAxisNames();

private:
  /// the vector describes default dimension names, specified along the axis if
  /// no names are explicitly requested;
  std::vector<std::string> m_DefaultDimID;
};
/** function to build mslice-like axis name from the vector, which describes
 * crystallographic direction along this axis*/
std::string DLLExport makeAxisName(const Kernel::V3D &vector,
                                   const std::vector<std::string> &Q1Names);
/**creates string representation of the number with accuracy, cpecified by eps*/
std::string DLLExport sprintfd(const double data, const double eps);

} // endnamespace MDAlgorithms
} // endnamespace Mantid

#endif