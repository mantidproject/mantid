#ifndef H_MDEVENT_WS_DESCRIPTION
#define H_MDEVENT_WS_DESCRIPTION

#include "MantidMDEvents/MDEvent.h"

namespace Mantid
{
namespace MDEvents
{
 /**  Lighteweith class wrapping together all parameters, related to MDEventoWorkspace used mainly to reduce number of parameters trasferred between 
    * an algorithm, creating MD workspace and UI.
    *   
    *   Introduced to decrease code bloat in methods and algorithms, which use MDEvents write interface and run-time defined number of dimensions
    
    @date 2011-28-12

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
/// helper class describes the properties of target MD workspace, which should be obtained as the result of conversion algorithm. 
  struct DLLExport MDWSDescription
  {
  public:
      /// constructor
     MDWSDescription():n_dims(0),emode(-1),Ei(std::numeric_limits<double>::quiet_NaN()){};
     /// mainly test constructor;
     MDWSDescription(size_t nDimesnions);
    /// the variable which describes the number of the dimensions, in the target workspace. 
    /// Calculated from number of input properties and the operations, performed on input workspace;
    size_t n_dims;
    /// conversion mode (see its description below)
    int emode;
    /// energy of incident neutrons, relevant in inelastic mode
    double Ei; 
    /// minimal and maximal values for the workspace dimensions:
    std::vector<double>      dim_min,dim_max;
    /// the names for the target workspace dimensions and properties of input MD workspace
    std::vector<std::string> dim_names;
    /// the units of target workspace dimensions and properties of input MD workspace dimensions
    std::vector<std::string> dim_units;
    /// the matrix to transform momentums of the workspace into notional target coordinate system
    std::vector<double> rotMatrix;  // should it be the Quat?
    /// helper function checks if min values are less them max values and are consistent between each other 
    void checkMinMaxNdimConsistent(Mantid::Kernel::Logger& log)const;
 
  }; 


}
}
#endif