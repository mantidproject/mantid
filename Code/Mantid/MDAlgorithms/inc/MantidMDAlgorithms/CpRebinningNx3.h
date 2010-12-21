#ifndef CP_REBINNING_Nx3_H
#define CP_REBINNING_Nx3_H
#include "MantidMDAlgorithms/DynamicCPRRebinning.h"
namespace Mantid
{
namespace MDAlgorithms 
{
 
    /**   Class does actual rebinning on N by 3 dataset 
       where N is number of dimensions and 3 -- number of reciprocal dimensions and calculates multidimensional 
       image and the locations of the points 

       Algorithm expects the target image to be clean and nullified -> strange results for
       signals (and incorrect errors) if not. 


        @author  Alex Buts,  ISIS RAL 
        @date 16/12/2010

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
class CpRebinningNx3: public DynamicCPRRebinning
{
public:

  CpRebinningNx3(const MDDataObjects::MDWorkspace_const_sptr &pSourceWS, 
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
       throw(Kernel::Exception::NotImplementedError("Not implemented at the moment but will be implemented soon"));
   };
  /** returns the estimate for number of data chunks may be used to rebin the dataset Used by algorithms to indicate progress*/
    virtual unsigned int getNumDataChunks()const;


  ~CpRebinningNx3();
protected:
// the variables used during rebinning operation
    int nDimensions;                     //< real number of dimensions in a dataset
    int nRecDims;                        //< number of reciprocal dimensions
    double rotations[9];                 //< rotation matrix for qx,qy,qz coordinates; 
    bool ignore_nan,ignore_inf;
    std::vector<double> shifts;          //< shift in all directions (tans_elo is 4th element of transf_bott_left
    std::vector<double> cut_min;         //< min limits to extract data;
    std::vector<double> cut_max;         //< max limits to extract data;
    std::vector<double> axis_step;       //< (cut_max-cut_min)/(nBins);
    std::vector<double> axis_step_inv;   //< 1/axis_step

    std::vector<size_t>  strides;
    std::vector<unsigned int> rec_dim_indexes; // the indexes of the reciprocal dimensions in the array of the target dimensions

    // working buffer to keep data pixels;
    std::vector<char  > pix_buf;
    /// build transformation matrix from the slicing data --> filled in all operation variables above
    virtual void build_scaled_transformation_matrix(const Geometry::MDGeometry &Source,const Geometry::MDGeometryDescription &target);
     /// first cell the rebinning process should begin
    size_t n_starting_cell;
    /// nuber of pixels read(processed) when rebinning 
    size_t n_pixels_read;
    /// (running) number of pixels selected to contribute into new dataset;
    size_t n_pixels_selected;
    /// number of pixels (datapoints, events) availible for rebinning
    size_t n_pix_in_buffer;
private:
  // the subroutine doing actual rebinning
    size_t rebin_Nx3dataset();
 
};
} // end namespaces
}

#endif