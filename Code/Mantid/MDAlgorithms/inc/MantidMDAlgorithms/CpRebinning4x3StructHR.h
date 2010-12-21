#ifndef CP_REBINNING_4x3_STRUCT_H
#define CP_REBINNING_4x3_STRUCT_H
#include "MantidMDAlgorithms/CpRebinningNx3.h"
namespace Mantid
{
namespace MDAlgorithms 
{
 
    /**   Class does actual rebinning on 4 by 3 dataset where the incoming data were presented as the structure, 
       simirlar to the Horace data structure. 
       where 4 is number of dimensions and 3 -- number of reciprocal dimensions.

       This is test class to check efficiency of a structure against more general datatype (MDDataPoint). I

        @author  Alex Buts,  ISIS RAL 
        @date 21/12/2010

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
class CpRebinning4x3StructHR: public CpRebinningNx3
{
public:
  /// the dataset expexted to work with specific data reader (hdfMatlab4D) and will throw if not provided with one
  CpRebinning4x3StructHR(const MDDataObjects::MDWorkspace_const_sptr &pSourceWS, 
                 Geometry::MDGeometryDescription const * const pTargetDescr,
                 const MDDataObjects::MDWorkspace_sptr  & TargetWS );
/*! function takes input multidimensional data points (pixels, events) stored in the source data buffer and 
     *  rebins these data (adds them) to MD image of the taget workspace;
     * Alternative (USA)vdescription: Identifies the locations of the datapoints in the multidimensional grid of the target workspace
    */
   virtual bool rebin_data_chunk();
  /** The same as just rebin_data_chunk above but the indexes returned as the last parameter specify the locations of the pixels
      * stored in the imput buffer;    */
   virtual bool rebin_data_chunk_keep_pixels(){
       throw(Kernel::Exception::NotImplementedError("Not implemented at the moment and may not be implemented"));
   };
private:
// the subroutine doing actual rebinning
    size_t rebin_4x3struct_dataset();
};
} // endnamespaces
}

#endif
