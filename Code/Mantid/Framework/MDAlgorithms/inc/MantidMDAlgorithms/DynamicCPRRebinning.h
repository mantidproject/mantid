#ifndef DYNAMIC_CPR_REBINNING_H
#define DYNAMIC_CPR_REBINNING_H

#include "MDDataObjects/MDWorkspace.h"
#include "MantidMDAlgorithms/IDynamicRebinning.h"
//
namespace Mantid
{
namespace MDAlgorithms
{
      /**  Class provides interface for centerpiece rebinning operations 
           and instantiates methods, common for centerpiece rebinning on regular grids
           CPR centerpiece, R- for regular, preselection  routine for irregular should certainly change
  

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
class DynamicCPRRebinning: public IDynamicRebinning
{

    
public:
    /// instantiated and virtual methods for rebinning, implemented in this class;
    virtual size_t preselect_cells(); 
    /** function returns the number of pixels which can contribute into a cut (Number of pixels in selected cells -- become valid after 
     *  preselection is done and precelected cells buffer is valid */
    virtual size_t getNumPreselectedPixels()const{return n_preselected_pix;}
    //
    virtual bool rebin_data_chunk()=0;
    virtual bool rebin_data_chunk_keep_pixels()=0;
   /** Calculates signals and errors of the MD image, obtained as the result of one or more rebin_dataset operations
     * and (in some implementations) the locations of the points in the final MDDatapoints array 
     *
     *   Returns the number of points (events, pixels) contributed into the image;    */
    virtual size_t finalize_rebinning();
 
    /** the constructor takes source workspace and the rebinning request and initiate proper target workspace, obtained from the dataservice
       It also retains shared pointers to source and target workspaces to work with; Childs will mainly work with parts of these workspaces
     */
    DynamicCPRRebinning(const MDDataObjects::MDWorkspace_const_sptr &pSourceWS, 
                        Geometry::MDGeometryDescription const * const pTargetDescr,
                        const MDDataObjects::MDWorkspace_sptr  & TargetWS );
      /// destructor 
    virtual ~DynamicCPRRebinning(){};
protected:
    //Do we need these? they can always be obtained from Mantid services;
    /// source workspace:
    MDDataObjects::MDWorkspace_const_sptr pSourceWS;
    /// target workspace
    MDDataObjects::MDWorkspace_sptr       pTargetWS;
    /** the description class, which describes final geometry and the cut. Necessary as 
     *  preselection works on the basis of this property and final geometry may not be initiated by constructor
     *  or can be entirely different */

    Geometry::MDGeometryDescription const * const pTargetDescr;

   /// Number of pixels which contributed into the target image
    size_t n_preselected_pix;
   /** The indexes of the sells of the source image that can contribute into the target image */
    //std::vector<const MDDataObjects::MD_image_point *const> preselected_cells;
    std::vector<size_t> preselected_cells;
 
 // pointer to the source image data --> provides number of pixels which may contribute into the cut;
    MDDataObjects::MDImage        const *const pSourceImg;
    // these are the parts of the source workspace
    Geometry::MDGeometry  const         *const pSourceGeom;  
   // pointer to the MD array of source image points
    MDDataObjects::MD_image_point const *const pSourceImgData; 

      /// pointer to the reader of the initial data -> TODO: - replace it with dataGetter from DataPixels
    MDDataObjects::IMD_FileFormat       *const pSourceDataReader;
    // pointer to the target geometry
    Geometry::MDGeometry                *      pTargetGeom;  
   // number of the cells in the target image 
    size_t    n_target_cells;
    // pointer to the MD array of target image points
    MDDataObjects::MD_image_point        *     pTargetImgData;

  
 ///*! function takes input multidimensional data points (pixels, events) stored in the source data buffer and 
 //    *  rebins these data (adds them) to MD image of the taget workspace;
 //    * Alternative (USA)vdescription: Identifies the locations of the datapoints in the multidimensional grid of the target workspace
 //    * and calculates the statistical properties of these points

 //    * Input arguments:
 //    @param source_pix_buf  -- the buffer where the data are stored as sequences of bytes;
 //    @param nPix            -- number of data points (pixels) contained in the buffer; 
 //                              The user of the rebinning class supposes to organise the interpretation of these bytes. 
 //     * Output arguments: 
 //       Returns              -- number of pixels(MDpoints, events) contribiting into the final dataset and retained from the initial data
 //   */
 //   virtual size_t rebin_data_chunk(const char *source_pix_buf, size_t nPix)=0;
 //   /** The same as just rebin_data_chunk with 2 arguments but the indexes returned as the last parameter specify the locations of the 
 //     * contribiting pixels in the cells of the target image; 
 //     TODO: There are different possibilities to identify the contributing points themselves: 
 //     1) put them into source buffer corrupting input data (nightmare for parralesation)
 //     2) introduse an "valid" bit on input data 
 //     3) Return an auxiliary boolean array specifying the valid pixels
 //     4) Rebin directly to target "pages" 
 //     All possibilities vary by complexety and efficiency so the best to identify soon
 //   */
 //   virtual size_t rebin_data(const char *source_pix_buf, size_t nPix, MDDataObjects::MDWorkspace &TargetWorkspace, 
 //                                   char * target_pix_buf, std::vector<size_t> &indexes)=0;

 //
 // 
 // /*! function returns the list of the cell numbers which can contribute into the cut described by the output geometry description
 // *  Input arguments:
 //    @param source           -- the geometry of the initial workspace
 //    @param target           -- 
 // *  Output arguments:
 //    @param cells_to_select  -- the list of the cell indexes, which can contribute into the cut
 //    @param n_preselected_pix-- number of pixels contributed into the cells. 

 //    As all rebinning classes currently use this function, it is here and not abstract at all; May change in a future as, say, sparce images 
 //    would like to overload it.
 //  */
 //   virtual void preselect_cells(const MDDataObjects::MDWorkspace &Source, const Geometry::MDGeometryDescription &target, std::vector<MDDataObjects::MD_image_point *> &cells_to_select,size_t &n_preselected_pix);

 
}; // end DynamicCPRRebinning

}
}

#endif