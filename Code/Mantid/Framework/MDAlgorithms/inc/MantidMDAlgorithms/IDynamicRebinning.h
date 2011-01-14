#ifndef IDYNAMIC_REBINNING_H
#define IDYNAMIC_REBINNING_H
#include "MantidKernel/Logger.h"
//
namespace Mantid
{
namespace MDAlgorithms
{
      /**   Class provides commont interface for various classes performing rebinning operations;

       A rebinning class is selected from availible classes which do rebinning operations on  
       user request and the ability to do the job. 
       
       TODO: A factory should analyse the demands for the job and user requests and return the method best suitable for the operations
             but the common interface for the rebinning can be identified and is described here. 


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
class IDynamicRebinning
{

    
public:
    /** Function identifies the indexes of the sells of the source image that can contribute into the target image
     *  returns number of selected cells and the number of pixels (datapoints, events) contained in these cells which can contribute
     * into the cut.     */
    virtual size_t preselect_cells()=0; 
  /*! function takes input multidimensional data points (pixels, events) stored in the source data buffer and 
     *  rebins these data (adds them) to MD image of the taget workspace;
     * Alternative (USA)vdescription: Identifies the locations of the datapoints in the multidimensional grid of the target workspace
     * and calculates the statistical properties of these points

      * Outputs: 
        Returns    true if more data availible and need to be rebinned;
    */
    virtual bool rebin_data_chunk()=0;
    /** The same as rebin_data_chunk but retains the datapoints (pixels) contributed in the image. This allows to save the image and
     *  further rebinning on basis of new MD workspace instead of the old one;
    */
    virtual bool rebin_data_chunk_keep_pixels()=0;
    /** returns the estimate for number of data chunks may be used to rebin the dataset Used by algorithms to 
        estimate the time to complete the rebinning*/
    virtual unsigned int getNumDataChunks()const=0;
    /** function returns the number of pixels which can contribute into a cut (Number of pixels in selected cells -- become valid after 
     *  preselection is done and precelected cells buffer is valid */
    virtual unsigned long getNumPreselectedPixels()const=0;


    /** Calculates signals and errors of the MD image, obtained as the result of one or more rebin_dataset operations
       and (in some implementations) the locations of the points in the final MDDatapoints array 

       Returns the number of points (events, pixels) contributed into the image;
     */
    virtual unsigned long finalize_rebinning()=0;
    /// destructor
    virtual ~IDynamicRebinning(){};
protected:
   /// logger -> to provide logging, for MD dataset file operations
    static Mantid::Kernel::Logger& bin_log;
}; // end IDynamicRebinning

}
}

#endif