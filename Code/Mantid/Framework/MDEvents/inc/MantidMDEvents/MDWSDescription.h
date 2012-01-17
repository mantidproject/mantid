#ifndef H_MDEVENT_WS_DESCRIPTION
#define H_MDEVENT_WS_DESCRIPTION

#include "MantidMDEvents/MDEvent.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

namespace Mantid
{
namespace MDEvents
{
 /**  Lighteweith class wrapping together all parameters, related to MDEventoWorkspace description used mainly to reduce number of parameters trasferred between 
    * an algorithm, creating MD workspace and UI.
    * It also defines some auxiliary functions, used for convenient description of MD workspace e.g. 
    *   
        
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
    /// the ID-s for the target workspace, which allow to identify the dimensions according to their ID
    std::vector<std::string> dim_IDs;
    /// the units of target workspace dimensions and properties of input MD workspace dimensions
    std::vector<std::string> dim_units;
    /** vectors, which describe the projection plain the target ws is based on (notional coordinate system). The transformation matrix below 
      * should bring the momentums from lab coordinate system into notional coordinate system */
    Kernel::V3D u,v;
    /// the indicator, informing if the uv plain has been set as a parameter. If they are not, the UB matrix from the source workspace is be used uncnanged
    bool is_uv_default;
    /// the matrix to transform momentums of the workspace into notional target coordinate system
    std::vector<double> rotMatrix;  // should it be the Quat?
    /// the oriented lattice which should be picked up from source ws and be carryed out to target ws as it can be modified by u,v on the way. 
    Geometry::OrientedLattice Latt;
    /// helper function checks if min values are less them max values and are consistent between each other 
    void checkMinMaxNdimConsistent(Mantid::Kernel::Logger& log)const;
    /// the vector of default names for Q-directrions in reciprocal space;
    std::vector<std::string> defailt_qNames;

  }; 
/** function to build mslice-like axis name from the vector, which describes crystallographic direction along this axis*/
std::string DLLExport makeAxisName(const Kernel::V3D &vector,const std::vector<std::string> &Q1Names);

}
}
#endif